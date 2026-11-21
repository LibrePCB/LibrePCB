//! TCP RPC server for external tools.
//!
//! This module implements an RPC server listening on a local TCP socket
//! (127.0.0.1, port assigned by the OS or by the caller). It is intended to
//! be the single entry point for external tools which want to interact
//! with a running LibrePCB instance, e.g. an MCP server, plugins or
//! scripting libraries. All those tools are just clients of this one
//! server.
//!
//! # Protocol
//!
//! The wire protocol is a custom, [MessagePack-RPC]-inspired design:
//! Messages are msgpack arrays `[type, ...]` with the message types
//! request (0), response (1), notification (2), request message (3) and
//! cancel (4), decoded directly from the TCP stream (self-delimiting, no
//! length prefix). See `dev/plans/rpc-server.md` for the full
//! specification.
//!
//! # Architecture
//!
//! * The whole protocol handling (framing, envelope) is done here in pure
//!   Rust, without any dependency to Qt or the C++ part of LibrePCB.
//! * Commands are dispatched through a [DispatchFn] callback, which is
//!   invoked with `(conn_id, msgid, method, params)` and returns
//!   immediately — the response is sent later through [RpcServer::reply].
//!   In the LibrePCB application the callback is implemented in C++
//!   (through the FFI, see `ffi/rpc_server_ffi.rs`) and marshals each
//!   command to the GUI thread, so command handlers can safely access
//!   editor state, and long-running commands can report progress with
//!   [RpcServer::notify_conn] before finally replying.
//! * This includes the `initialize` request: The authentication (token
//!   validation) is implemented on the C++ side, which then calls
//!   [RpcServer::set_authenticated] to let the server know whether the
//!   connection is authenticated. While a connection is not
//!   authenticated, requests other than `initialize` are rejected with an
//!   error instead of being dispatched.
//! * The msgid is only unique per connection, so all reply functions
//!   require the connection ID in addition to the msgid.
//! * The server tracks pending requests (dispatched, but not yet replied)
//!   per connection and enforces the protocol invariants: exactly one
//!   response per request, and request messages ([RpcServer::notify_conn])
//!   only while the request is pending.
//! * Events can be broadcast to all connected clients with
//!   [RpcServer::notify_all], as notifications.
//!
//! # Threading Model
//!
//! No async runtime is used, just plain std threads (the number of local
//! clients is small, so this is by far the simplest and most maintainable
//! option):
//!
//! * The accept loop runs on its own thread, polling the non-blocking
//!   listener. This makes it trivially interruptible on every platform
//!   when stopping the server.
//! * Each connection consists of two threads: A reader thread which
//!   blocks on decoding messages from the stream and dispatches them, and
//!   a writer thread which blocks on a channel receiving the encoded
//!   messages to send. This decouples blocking writes from reading, and
//!   allows other threads (e.g. the GUI thread) to push responses and
//!   notifications at any time.
//! * Requests are dispatched **asynchronously** (fire-and-forget): the
//!   reader thread continues reading (and dispatching) further requests
//!   while previous ones are still being processed, and responses may be
//!   sent in any order (pipelining, as in the official msgpack-rpc spec).
//!   All bookkeeping (pending msgids) is synchronized internally.
//!
//! [MessagePack-RPC]: https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md

mod codec;
mod connection;
mod protocol;

// Re-Exports
pub use protocol::{RpcError, RpcErrorCode};

use std::collections::{HashMap, HashSet};
use std::io;
use std::net::{Shutdown, TcpListener, TcpStream};
use std::sync::atomic::{AtomicBool, AtomicU64, AtomicUsize, Ordering};
use std::sync::mpsc::Sender;
use std::sync::{Arc, Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

use protocol::{build_notification, build_response};
use rmpv::Value;

/// Maximum time [RpcServer::stop] waits for connection threads to finish.
/// They cannot be interrupted while blocked inside a dispatch callback, so
/// after this timeout they are detached with a warning (which is fine
/// during application teardown).
const STOP_TIMEOUT: Duration = Duration::from_secs(5);

/// The outcome of a [DispatchFn] call.
#[derive(Debug)]
pub enum DispatchOutcome {
  /// The request was handed over to the command handler; the response will
  /// be sent asynchronously via [RpcServer::reply].
  Dispatched,
  /// The request could not be dispatched (e.g. the params could not be
  /// converted at the FFI boundary); the connection code replies with this
  /// error immediately.
  ReplyNow(RpcError),
}

/// Callback which handles a command request (fire-and-forget).
///
/// It receives the connection ID, the msgid, the method name (e.g.
/// `"Project.list"`) and the raw msgpack params (a map, opaque to this
/// server module — its content is only interpreted by the command
/// handlers). If the request can be processed, the callback returns
/// [DispatchOutcome::Dispatched] and ensures that
/// [RpcServer::reply] is called exactly once, at any time, from any
/// thread. Otherwise it returns [DispatchOutcome::ReplyNow] to let the
/// connection code answer with an error.
///
/// The callback is invoked on the connection's reader thread, i.e.
/// possibly concurrently from multiple threads, but strictly sequentially
/// per connection.
///
/// Note that the `initialize` request is dispatched through this callback
/// as well: The authentication (token validation) is implemented on the
/// C++ side, which then calls [RpcServer::set_authenticated] to let the
/// server know whether the connection is authenticated now.
pub type DispatchFn =
  Arc<dyn Fn(u64, u32, &str, &Value) -> DispatchOutcome + Send + Sync>;

/// Callback which is invoked when a client cancels a pending request
/// (`[4, msgid]`).
///
/// The callback must eventually make sure the request is replied exactly
/// once (typically with a "cancelled" error, or with the normal result if
/// the work already completed).
pub type CancelFn = Arc<dyn Fn(u64, u32) + Send + Sync>;

/// Internal shared state of the [RpcServer], used by all its threads.
struct Inner {
  /// The command dispatch callback (see [DispatchFn]).
  dispatch: DispatchFn,
  /// The cancel callback (see [CancelFn]).
  cancel: CancelFn,
  /// Registered connections, by connection ID.
  clients: Mutex<HashMap<u64, Connection>>,
  /// Whether the server is running (set to `false` by [RpcServer::stop]).
  running: AtomicBool,
  /// Number of currently active connections (reader threads).
  active_connections: AtomicUsize,
  /// Generator for connection IDs.
  next_conn_id: AtomicU64,
  /// The handle of the accept loop thread, if the server was started.
  accept_thread: Mutex<Option<JoinHandle<()>>>,
}

/// A registered connection, used to push messages to it, to shut it down
/// and to track its pending requests.
struct Connection {
  /// Channel to the connection's writer thread (messages are already
  /// encoded to bytes).
  tx: Sender<Vec<u8>>,
  /// Clone of the connection's socket, used to shut it down when stopping
  /// the server (which unblocks both the reader and the writer thread).
  stream: TcpStream,
  /// The msgids of requests which were dispatched but not yet replied.
  pending: Arc<Mutex<HashSet<u32>>>,
  /// Whether the connection is authenticated. Set by the C++ side through
  /// [RpcServer::set_authenticated] (the token validation lives there).
  /// While this is `false`, requests other than `initialize` are rejected
  /// with an error instead of being dispatched.
  authenticated: Arc<AtomicBool>,
}

/// The RPC server.
///
/// Usage: Create it with [RpcServer::new], then call [RpcServer::start] to
/// make it listen on 127.0.0.1 (the OS assigns the port, which is
/// returned). Push events to all connected clients with
/// [RpcServer::notify_all], and shut the server down with
/// [RpcServer::stop] (or just drop it, which does the same).
///
/// All methods can be called from any thread. The dispatch and cancel
/// callbacks are invoked on the connection's reader threads.
pub struct RpcServer {
  /// The state shared between the server and all its threads.
  inner: Arc<Inner>,
}

impl RpcServer {
  /// Create a new server (not started yet).
  pub fn new(dispatch: DispatchFn, cancel: CancelFn) -> Self {
    Self {
      inner: Arc::new(Inner {
        dispatch,
        cancel,
        clients: Mutex::new(HashMap::new()),
        running: AtomicBool::new(false),
        active_connections: AtomicUsize::new(0),
        next_conn_id: AtomicU64::new(0),
        accept_thread: Mutex::new(None),
      }),
    }
  }

  /// Start listening on 127.0.0.1, with the port assigned by the OS.
  ///
  /// Returns the chosen port, which the caller needs to make the server
  /// discoverable for clients. Returns an error if the server is already
  /// running.
  pub fn start(&self) -> io::Result<u16> {
    self.start_on_port(0)
  }

  /// Start listening on 127.0.0.1 on the given port (0 = OS-assigned).
  ///
  /// Returns the chosen port, which the caller needs to make the server
  /// discoverable for clients. Returns an error if the server is already
  /// running.
  pub fn start_on_port(&self, port: u16) -> io::Result<u16> {
    if self.inner.running.load(Ordering::SeqCst) {
      return Err(io::Error::new(
        io::ErrorKind::InvalidInput,
        "server is already running",
      ));
    }
    let listener = TcpListener::bind(("127.0.0.1", port))?;
    listener.set_nonblocking(true)?;
    let port = listener.local_addr()?.port();
    self.inner.running.store(true, Ordering::SeqCst);
    let inner = self.inner.clone();
    let handle = thread::Builder::new()
      .name("rpc-accept".to_string())
      .spawn(move || connection::accept_loop(inner, listener))?;
    *lock(&self.inner.accept_thread) = Some(handle);
    Ok(port)
  }

  /// Send the response for a pending request.
  ///
  /// Exactly one of `error`/`result` is sent (with an error, the result is
  /// nil, and vice versa, as the protocol requires). A second response for
  /// the same msgid is silently dropped (returning `false`), as is a reply
  /// for an unknown or already closed connection — this enforces the
  /// "exactly one response per request" protocol invariant.
  ///
  /// This method never blocks (the message is just queued on the
  /// connection's writer thread) and may be called from any thread.
  ///
  /// Returns whether the response was sent, i.e. whether the request was
  /// actually pending.
  pub fn reply(
    &self,
    conn_id: u64,
    msgid: u32,
    error: Option<&RpcError>,
    result: &Value,
  ) -> bool {
    let error = error.map(|e| e.to_value());
    let message = build_response(msgid, error.as_ref(), result);
    self.send_response(conn_id, msgid, &message)
  }

  /// Send the response for a pending request, passing a raw error map
  /// through unchanged.
  ///
  /// Used by the FFI for errors produced by the C++ dispatch layer: Their
  /// map may carry additional error properties next to "code" and
  /// "message" (see the error types in the API specification), so it is
  /// forwarded as-is instead of being rebuilt from an [RpcError].
  pub fn reply_error(&self, conn_id: u64, msgid: u32, error: &Value) -> bool {
    let message = build_response(msgid, Some(error), &Value::Nil);
    self.send_response(conn_id, msgid, &message)
  }

  /// Look up the connection, remove the msgid from the pending set and
  /// queue the encoded message on its writer thread (see [RpcServer::reply]).
  fn send_response(&self, conn_id: u64, msgid: u32, message: &Value) -> bool {
    let conn = {
      let clients = lock(&self.inner.clients);
      clients
        .get(&conn_id)
        .map(|conn| (conn.tx.clone(), Arc::clone(&conn.pending)))
    };
    let Some((tx, pending)) = conn else {
      return false;
    };
    // Removing the msgid from the pending set must happen atomically with
    // the response being queued, so that no request message (type 3) can
    // slip in between (see notify_conn).
    let mut pending = lock(&pending);
    if !pending.remove(&msgid) {
      return false; // Already replied (or never dispatched).
    }
    drop(pending);
    match codec::encode_message(message) {
      Ok(bytes) => tx.send(bytes).is_ok(),
      Err(e) => {
        eprintln!("librepcb-rust: Failed to encode a response: {e}");
        false
      }
    }
  }

  /// Send a request-scoped message (message type 3, e.g. a progress report
  /// or streamed partial data) to the client which sent the request.
  ///
  /// The message is only delivered while the request is pending, i.e.
  /// before its response was sent (returns `false` otherwise). Like
  /// [RpcServer::reply], this never blocks and may be called from any
  /// thread.
  pub fn notify_conn(&self, conn_id: u64, msgid: u32, payload: &Value) -> bool {
    let conn = {
      let clients = lock(&self.inner.clients);
      clients
        .get(&conn_id)
        .map(|conn| (conn.tx.clone(), Arc::clone(&conn.pending)))
    };
    let Some((tx, pending)) = conn else {
      return false;
    };
    // Holding the pending lock while queuing the message guarantees that
    // no response can be sent concurrently (which would make this message
    // arrive after the response, violating the protocol).
    let pending = lock(&pending);
    if !pending.contains(&msgid) {
      return false; // Not (or no longer) pending.
    }
    let message = protocol::build_request_message(msgid, payload);
    match codec::encode_message(&message) {
      Ok(bytes) => tx.send(bytes).is_ok(),
      Err(e) => {
        eprintln!("librepcb-rust: Failed to encode a request message: {e}");
        false
      }
    }
  }

  /// Push an event (a notification, message type 2) to all connected
  /// clients.
  ///
  /// This method never blocks (the message is just queued on each
  /// connection) and thus may safely be called while a command dispatch is
  /// in progress.
  pub fn notify_all(&self, method: &str, params: &Value) {
    let message = build_notification(method, params);
    let Ok(bytes) = codec::encode_message(&message) else {
      eprintln!("librepcb-rust: Failed to encode a notification");
      return;
    };
    let mut clients = lock(&self.inner.clients);
    let mut dead = Vec::new();
    for (id, client) in clients.iter() {
      if client.tx.send(bytes.clone()).is_err() {
        dead.push(*id);
      }
    }
    for id in dead {
      clients.remove(&id);
    }
  }

  /// Mark a connection as (un)authenticated.
  ///
  /// Called by the C++ side after it validated the authentication token of
  /// an `initialize` request (the token handling lives on the C++ side).
  /// While a connection is not authenticated, requests other than
  /// `initialize` are rejected with an error instead of being dispatched.
  ///
  /// Calls for unknown or already closed connections are ignored.
  pub fn set_authenticated(&self, conn_id: u64, authenticated: bool) {
    let clients = lock(&self.inner.clients);
    if let Some(conn) = clients.get(&conn_id) {
      conn.authenticated.store(authenticated, Ordering::SeqCst);
    }
  }

  /// Stop the server.
  ///
  /// Closes the listener and all client sockets and waits (up to 5
  /// seconds) for the connection threads to finish. Threads which are
  /// currently blocked inside a dispatch callback cannot be interrupted,
  /// so after the timeout they are detached with a warning (which is fine
  /// during application teardown). Calling this on a stopped server is a
  /// no-op.
  pub fn stop(&self) {
    self.inner.running.store(false, Ordering::SeqCst);
    // The accept loop polls the running flag, so it (and thus also this
    // join) returns within ~10 ms. It must not be running when the client
    // sockets are shut down below, otherwise new connections could sneak
    // in between.
    if let Some(handle) = lock(&self.inner.accept_thread).take() {
      let _ = handle.join();
    }
    // Shutting down the sockets unblocks the reader threads (EOF) and the
    // writer threads (write error or closed channel).
    {
      let mut clients = lock(&self.inner.clients);
      for client in clients.values() {
        let _ = client.stream.shutdown(Shutdown::Both);
      }
      clients.clear();
    }
    let deadline = Instant::now() + STOP_TIMEOUT;
    while self.inner.active_connections.load(Ordering::SeqCst) > 0 {
      if Instant::now() >= deadline {
        eprintln!(
          "librepcb-rust: Detaching connection threads which are still busy"
        );
        break;
      }
      thread::sleep(Duration::from_millis(10));
    }
  }
}

impl Drop for RpcServer {
  fn drop(&mut self) {
    self.stop();
  }
}

/// Lock a mutex, ignoring a poisoned state (a panic in another thread
/// doesn't justify failing the whole server).
fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
  mutex.lock().unwrap_or_else(|e| e.into_inner())
}

#[cfg(test)]
mod tests {
  use super::*;
  use protocol::{METHOD_INITIALIZE, METHOD_PING};
  use std::io::{BufReader, Write};
  use std::sync::mpsc;

  /// The token the test's emulated C++ initialize handler accepts.
  const TOKEN: &str = "0123456789abcdef0123456789abcdef";

  /// A TCP client connected to the test server, speaking the msgpack
  /// protocol.
  struct TestClient {
    reader: BufReader<TcpStream>,
    writer: TcpStream,
  }

  impl TestClient {
    /// Connect without authenticating.
    fn new_unauthenticated(port: u16) -> Self {
      let stream = TcpStream::connect(("127.0.0.1", port)).unwrap();
      // Timeout to avoid hanging forever (and thus blocking CI) if the
      // server does not respond.
      stream
        .set_read_timeout(Some(Duration::from_secs(10)))
        .unwrap();
      Self {
        reader: BufReader::new(stream.try_clone().unwrap()),
        writer: stream,
      }
    }

    /// Connect and authenticate: Send the `initialize` request with the
    /// test token, emulate the C++ initialize handler on the forwarded
    /// request (validate token, mark authenticated, reply) and read the
    /// response.
    fn new(port: u16, server: &RpcServer, dispatch: &TestDispatch) -> Self {
      let mut client = Self::new_unauthenticated(port);
      client.send_initialize(TOKEN);
      assert!(
        emulate_cpp_initialize(server, dispatch, TOKEN),
        "initialize failed"
      );
      let (error, _) = client.read_response(1);
      assert!(error.is_nil(), "initialize failed");
      client
    }

    /// Send a raw (already encoded) message.
    fn send_raw(&mut self, bytes: &[u8]) {
      self.writer.write_all(bytes).unwrap();
      self.writer.flush().unwrap();
    }

    /// Half-close the connection (signals EOF to the server, while still
    /// being able to read its response).
    fn shutdown_write(&mut self) {
      self.writer.shutdown(std::net::Shutdown::Write).unwrap();
    }

    /// Send a message (one msgpack value).
    fn send(&mut self, value: &Value) {
      let bytes = codec::encode_message(value).unwrap();
      self.send_raw(&bytes);
    }

    /// Send an `initialize` request with the given token.
    fn send_initialize(&mut self, token: &str) {
      self.send(&Self::request_value(
        1,
        METHOD_INITIALIZE,
        Value::Map(vec![(Value::from("token"), Value::from(token))]),
      ));
    }

    /// Build a request message.
    fn request_value(msgid: u32, method: &str, params: Value) -> Value {
      Value::Array(vec![
        Value::from(0),
        Value::from(msgid),
        Value::from(method),
        params,
      ])
    }

    /// Send a request.
    fn request(&mut self, msgid: u32, method: &str, params: Value) {
      self.send(&Self::request_value(msgid, method, params));
    }

    /// Read one message (blocking, with timeout). Returns `None` on EOF,
    /// i.e. if the server closed the connection.
    fn read(&mut self) -> Option<Value> {
      codec::read_message(&mut self.reader)
        .unwrap_or_else(|e| panic!("failed to decode a message: {e}"))
    }

    /// Read a response, asserting that it is a response for the given
    /// msgid. Returns `(error, result)` — exactly one of them is non-nil.
    fn read_response(&mut self, msgid: u32) -> (Value, Value) {
      let message = self.read().expect("connection closed unexpectedly");
      let items = message.as_array().unwrap();
      assert_eq!(items[0], Value::from(1), "not a response: {message:?}");
      assert_eq!(
        items[1],
        Value::from(msgid),
        "unexpected msgid: {message:?}"
      );
      assert_eq!(items.len(), 4, "invalid response: {message:?}");
      (items[2].clone(), items[3].clone())
    }

    /// Read a response, asserting that it contains the given error code.
    fn read_error(&mut self, msgid: u32, code: RpcErrorCode) {
      let (error, result) = self.read_response(msgid);
      assert!(result.is_nil(), "expected an error, got a result");
      assert_eq!(
        error.as_map().unwrap()[0],
        (Value::from("code"), Value::from(code as i64)),
      );
    }
  }

  /// A dispatch handler which forwards all requests into a channel (to be
  /// replied by the test code, just like the C++ integration would).
  type TestDispatch = mpsc::Receiver<(u64, u32, String, Value)>;

  /// Create a dispatch + cancel callback pair which forward the calls
  /// into channels.
  fn forwarding_handlers() -> (
    DispatchFn,
    CancelFn,
    TestDispatch,
    mpsc::Receiver<(u64, u32)>,
  ) {
    let (dispatch_tx, dispatch_rx) = mpsc::channel();
    let (cancel_tx, cancel_rx) = mpsc::channel();
    (
      Arc::new(
        move |conn_id: u64, msgid: u32, method: &str, params: &Value| {
          dispatch_tx
            .send((conn_id, msgid, method.to_string(), params.clone()))
            .unwrap();
          DispatchOutcome::Dispatched
        },
      ),
      Arc::new(move |conn_id: u64, msgid: u32| {
        cancel_tx.send((conn_id, msgid)).unwrap();
      }),
      dispatch_rx,
      cancel_rx,
    )
  }

  /// Create a started test server with the given handlers.
  fn spawn_server_with_handlers(
    dispatch: DispatchFn,
    cancel: CancelFn,
  ) -> (RpcServer, u16) {
    let server = RpcServer::new(dispatch, cancel);
    let port = server.start().unwrap();
    (server, port)
  }

  /// Create a started test server with forwarding handlers.
  fn spawn_server() -> (RpcServer, u16, TestDispatch, mpsc::Receiver<(u64, u32)>)
  {
    let (dispatch, cancel, dispatch_rx, cancel_rx) = forwarding_handlers();
    let (server, port) = spawn_server_with_handlers(dispatch, cancel);
    (server, port, dispatch_rx, cancel_rx)
  }

  /// Emulate the C++ initialize handler on a forwarded request: Validate
  /// the token, mark the connection as (un)authenticated and reply.
  /// Returns whether the token was accepted.
  fn emulate_cpp_initialize(
    server: &RpcServer,
    dispatch: &TestDispatch,
    expected_token: &str,
  ) -> bool {
    let (conn_id, msgid, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_INITIALIZE);
    let token = params
      .as_map()
      .unwrap()
      .iter()
      .find(|(k, _)| k.as_str() == Some("token"))
      .map(|(_, v)| v.as_str().unwrap().to_string());
    let authenticated = token.as_deref() == Some(expected_token);
    server.set_authenticated(conn_id, authenticated);
    if authenticated {
      server.reply(conn_id, msgid, None, &Value::Map(Vec::new()));
    } else {
      server.reply(
        conn_id,
        msgid,
        Some(&RpcError::new(
          RpcErrorCode::Unauthenticated,
          "Invalid or missing token",
        )),
        &Value::Nil,
      );
    }
    authenticated
  }

  /// Get a field of a map value.
  fn field<'a>(value: &'a Value, key: &str) -> Option<&'a Value> {
    value
      .as_map()?
      .iter()
      .find(|(k, _)| k.as_str() == Some(key))
      .map(|(_, v)| v)
  }

  #[test]
  fn test_initialize() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize(TOKEN);
    // The request is dispatched to the handler, with the token in the
    // params (the whole authentication is up to the C++ side):
    let (conn_id, msgid, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_INITIALIZE);
    assert_eq!(field(&params, "token"), Some(&Value::from(TOKEN)));
    // The C++ side validates the token, marks the connection as
    // authenticated and replies with the application information:
    server.set_authenticated(conn_id, true);
    server.reply(conn_id, msgid, None, &Value::Map(Vec::new()));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
    // The connection is now usable:
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    let (error, result) = client.read_response(2);
    assert!(error.is_nil());
    assert_eq!(result, Value::Map(Vec::new()));
  }

  #[test]
  fn test_initialize_with_wrong_token() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize("wrong");
    // The C++ side rejects the wrong token. The connection stays open,
    // but non-initialize requests are rejected:
    assert!(!emulate_cpp_initialize(&server, &dispatch, TOKEN));
    client.read_error(1, RpcErrorCode::Unauthenticated);
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(2, RpcErrorCode::Unauthenticated);
  }

  #[test]
  fn test_initialize_with_wrong_token_after_auth() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize(TOKEN);
    assert!(emulate_cpp_initialize(&server, &dispatch, TOKEN));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
    // Re-initialize with a wrong token → the C++ side un-authenticates the
    // connection and replies with an error. The connection stays open, but
    // further (non-initialize) requests are rejected:
    client.send_initialize("wrong");
    assert!(!emulate_cpp_initialize(&server, &dispatch, TOKEN));
    client.read_error(1, RpcErrorCode::Unauthenticated);
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(2, RpcErrorCode::Unauthenticated);
    // However, initializing correctly again works:
    client.send_initialize(TOKEN);
    assert!(emulate_cpp_initialize(&server, &dispatch, TOKEN));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
  }

  #[test]
  fn test_first_request_not_initialize() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    // A non-initialize request on an unauthenticated connection is rejected
    // without being dispatched, and the connection stays open:
    client.request(1, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(1, RpcErrorCode::Unauthenticated);
    assert!(dispatch.try_recv().is_err());
    // Initializing afterwards works:
    client.send_initialize(TOKEN);
    assert!(emulate_cpp_initialize(&server, &dispatch, TOKEN));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
  }

  #[test]
  fn test_ping() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(5, METHOD_PING, Value::Map(Vec::new()));
    let (error, result) = client.read_response(5);
    assert!(error.is_nil());
    assert_eq!(result, Value::Map(Vec::new()));
  }

  #[test]
  fn test_dispatch_and_reply() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    let params = Value::Map(vec![
      (Value::from("obj"), Value::from("some-uuid")),
      (Value::from("x"), Value::from(1)),
    ]);
    client.request(42, "Project.get", params.clone());
    // The dispatch handler received the request...
    let (conn_id, msgid, method, received_params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(msgid, 42);
    assert_eq!(method, "Project.get");
    assert_eq!(received_params, params);
    // ...and we reply asynchronously, just like the C++ integration does:
    assert!(server.reply(
      conn_id,
      msgid,
      None,
      &Value::Map(vec![(Value::from("name"), Value::from("Test"))])
    ));
    let (error, result) = client.read_response(42);
    assert!(error.is_nil());
    assert_eq!(field(&result, "name"), Some(&Value::from("Test")));
  }

  #[test]
  fn test_dispatch_reply_now_error() {
    // A handler which rejects every request with ReplyNow (e.g. params
    // conversion failures at the FFI boundary). Since `initialize` is
    // rejected as well, the connection can't be authenticated — which is
    // fine for this test.
    let dispatch: DispatchFn = Arc::new(|_, _, _, _| {
      DispatchOutcome::ReplyNow(RpcError::invalid_params())
    });
    let cancel: CancelFn = Arc::new(|_, _| {});
    let (_server, port) = spawn_server_with_handlers(dispatch, cancel);
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize(TOKEN);
    client.read_error(1, RpcErrorCode::InvalidParams);
  }

  #[test]
  fn test_exactly_one_reply() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // The first reply is sent, a second one for the same msgid is dropped:
    assert!(server.reply(conn_id, msgid, None, &Value::Nil));
    assert!(!server.reply(conn_id, msgid, None, &Value::Nil));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
    // No second response arrives, i.e. the next message is the ping
    // response:
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    client.read_response(2);
  }

  #[test]
  fn test_reply_for_unknown_msgid() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // A reply for a msgid which was never dispatched:
    assert!(!server.reply(u64::MAX, 123, None, &Value::Nil));
    // A reply for a valid connection, but an unknown msgid:
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert!(!server.reply(conn_id, msgid + 100, None, &Value::Nil));
    // The pending request is still pending, i.e. it can be replied:
    assert!(server.reply(conn_id, msgid, None, &Value::Nil));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
  }

  #[test]
  fn test_reply_after_connection_closed() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    drop(client); // Close the connection.
                  // Give the server a moment to process the disconnect.
    thread::sleep(Duration::from_millis(100));
    // The reply is silently dropped instead of panicking:
    assert!(!server.reply(conn_id, msgid, None, &Value::Nil));
  }

  #[test]
  fn test_notify_conn_only_while_pending() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // While the request is pending, request messages (type 3) arrive:
    let progress = Value::Map(vec![
      (Value::from("percent"), Value::from(50)),
      (Value::from("message"), Value::from("Working")),
    ]);
    assert!(server.notify_conn(conn_id, msgid, &progress));
    let message = client.read().unwrap();
    let items = message.as_array().unwrap();
    assert_eq!(items[0], Value::from(3));
    assert_eq!(items[1], Value::from(1));
    assert_eq!(field(&items[2], "percent"), Some(&Value::from(50)));
    // After the response was sent, no further messages are accepted:
    assert!(server.reply(conn_id, msgid, None, &Value::Nil));
    assert!(!server.notify_conn(conn_id, msgid, &progress));
    client.read_response(1);
  }

  #[test]
  fn test_notify_all() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client1 = TestClient::new(port, &server, &dispatch);
    let mut client2 = TestClient::new(port, &server, &dispatch);
    let params = Value::Map(vec![(Value::from("a"), Value::from(1))]);
    server.notify_all("project_opened", &params);
    for client in [&mut client1, &mut client2] {
      let message = client.read().unwrap();
      let items = message.as_array().unwrap();
      assert_eq!(items[0], Value::from(2));
      assert_eq!(items[1], Value::from("project_opened"));
      assert_eq!(field(&items[2], "a"), Some(&Value::from(1)));
    }
  }

  #[test]
  fn test_notify_all_with_nil_params() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    server.notify_all("event", &Value::Nil);
    let message = client.read().unwrap();
    assert_eq!(message.as_array().unwrap()[2], Value::Nil);
  }

  #[test]
  fn test_cancel_is_forwarded() {
    let (server, port, dispatch, cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // Cancel the pending request → the cancel handler is invoked...
    client.send(&Value::Array(vec![Value::from(4), Value::from(1)]));
    let (cancelled_conn, cancelled_msgid) =
      cancel.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!((cancelled_conn, cancelled_msgid), (conn_id, msgid));
    // ...and we reply with a "cancelled" error, as the C++ side would:
    assert!(server.reply(
      conn_id,
      msgid,
      Some(&RpcError::new(RpcErrorCode::Cancelled, "cancelled")),
      &Value::Nil
    ));
    client.read_error(1, RpcErrorCode::Cancelled);
  }

  #[test]
  fn test_cancel_for_unknown_msgid_is_ignored() {
    let (server, port, dispatch, cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // Cancel a msgid which was never dispatched: ignored (no cancel
    // callback, no response) — the next message is the ping response.
    client.send(&Value::Array(vec![Value::from(4), Value::from(123)]));
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    client.read_response(2);
    assert!(cancel.try_recv().is_err());
  }

  #[test]
  fn test_pipelining_requests() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // Send three requests without waiting for responses. Since dispatch
    // is asynchronous, all of them are dispatched before any reply:
    for msgid in 1..=3u32 {
      client.request(msgid, "Project.list", Value::Map(Vec::new()));
    }
    let mut requests = Vec::new();
    for _ in 0..3 {
      requests.push(dispatch.recv_timeout(Duration::from_secs(10)).unwrap());
    }
    // Reply in reverse order — responses may arrive in any order:
    for (conn_id, msgid, _, _) in requests.iter().rev() {
      assert!(server.reply(*conn_id, *msgid, None, &Value::from(*msgid)));
    }
    for msgid in (1..=3u32).rev() {
      let (error, result) = client.read_response(msgid);
      assert!(error.is_nil());
      assert_eq!(result, Value::from(msgid));
    }
  }

  #[test]
  fn test_duplicate_msgid_is_rejected() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project.list", Value::Map(Vec::new()));
    let (conn_id, msgid, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // A second request with the same msgid while the first one is still
    // pending is a protocol violation and answered with an error, without
    // being dispatched:
    client.request(1, "Project.list", Value::Map(Vec::new()));
    client.read_error(1, RpcErrorCode::InvalidRequest);
    // The first request is still pending and can be replied:
    assert!(server.reply(conn_id, msgid, None, &Value::Nil));
    let (error, _) = client.read_response(1);
    assert!(error.is_nil());
  }

  #[test]
  fn test_set_authenticated_for_unknown_connection() {
    let (server, _port, _dispatch, _cancel) = spawn_server();
    // Calls for unknown connections are ignored (must not panic):
    server.set_authenticated(u64::MAX, true);
    server.set_authenticated(u64::MAX, false);
  }

  #[test]
  fn test_stop_closes_connections() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    server.stop();
    assert!(client.read().is_none());
    // Stopping again is a no-op (and must not hang):
    server.stop();
  }

  #[test]
  fn test_stop_without_start() {
    let (dispatch, cancel, _dispatch_rx, _cancel_rx) = forwarding_handlers();
    let server = RpcServer::new(dispatch, cancel);
    server.stop(); // Must be a no-op.
  }

  #[test]
  fn test_drop_stops_server() {
    let mut client;
    {
      let (server, port, dispatch, _cancel) = spawn_server();
      client = Some(TestClient::new(port, &server, &dispatch));
      // The server is dropped here (at the end of the block).
    }
    assert!(client.as_mut().unwrap().read().is_none());
  }

  #[test]
  fn test_start_twice_is_rejected() {
    let (server, _port, _dispatch, _cancel) = spawn_server();
    assert!(server.start().is_err());
  }
}
