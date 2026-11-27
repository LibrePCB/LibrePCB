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
//! The wire protocol is a custom, [MessagePack-RPC]-inspired design: It is
//! *bidirectional* — both sides of a connection can initiate method calls
//! in exactly the same way. Messages are msgpack arrays `[type, ...]` with
//! the message types request (0), response (1), request message (2) and
//! cancel (3), decoded directly from the TCP stream (self-delimiting, no
//! length prefix). See `dev/plans/rpc-server.md` for the full
//! specification.
//!
//! # Architecture
//!
//! * The whole protocol handling (framing, envelope) is done here in pure
//!   Rust, without any dependency to Qt or the C++ part of LibrePCB.
//! * Commands are dispatched through a [DispatchFn] callback, which is
//!   invoked with `(conn_id, msg_id, method, params)` and returns
//!   immediately — the response is sent later through [RpcServer::reply].
//!   In the LibrePCB application the callback is implemented in C++
//!   (through the FFI, see `ffi/rpc_server_ffi.rs`) and marshals each
//!   command to the GUI thread, so command handlers can safely access
//!   editor state, and long-running commands can report progress with
//!   [RpcServer::notify_conn] before finally replying.
//! * This includes the `initialize` and `authenticate` requests: The
//!   authentication (key/token validation) is implemented on the C++ side,
//!   and the reply to an `authenticate` request implicitly
//!   (un-)authenticates the connection (success/error). While a connection
//!   is not authenticated, requests other than those two are rejected with
//!   an error instead of being dispatched.
//! * The msg_id is only unique per connection, so all reply functions
//!   require the connection ID in addition to the msg_id.
//! * The server tracks pending requests (dispatched, but not yet replied)
//!   per connection and enforces the protocol invariants: exactly one
//!   response per request, and request messages ([RpcServer::notify_conn])
//!   only while the request is pending.
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
//!   All bookkeeping (pending msg_ids) is synchronized internally.
//!
//! [MessagePack-RPC]: https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md

mod codec;
mod connection;
mod protocol;
mod rpc_api;

// Re-Exports
pub use protocol::RpcError;
pub use rpc_api::RpcErrorCode;

use std::collections::{HashMap, HashSet};
use std::io;
use std::net::{Shutdown, TcpListener, TcpStream};
use std::sync::atomic::{AtomicBool, AtomicU32, AtomicUsize, Ordering};
use std::sync::mpsc::Sender;
use std::sync::{Arc, Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

use protocol::build_response;
use rmpv::Value;

/// The default TCP port of the RPC server: The letters "LP" (for
/// LibrePCB) interpreted as a hexadecimal number: 0x4C50 = 19536.
///
/// If the port is already in use (e.g. by another LibrePCB instance), the
/// server binds the next higher port instead (see [MAX_PORT_ATTEMPTS]).
/// The port is in the IANA registered range, but (to the best of our
/// knowledge) unassigned, and it's below the ephemeral port ranges of
/// all major platforms, so it doesn't collide with outgoing connections.
const DEFAULT_PORT: u16 = 19536; // "LP" = 0x4C50

/// Maximum number of port binding attempts: The default port plus this
/// many successors (see [DEFAULT_PORT]). Binding is a cheap syscall, so
/// trying many ports is negligible.
const MAX_PORT_ATTEMPTS: u16 = 100;

/// Maximum number of *simultaneously active* connections. Additional
/// connections are closed immediately (the client just sees the socket
/// being closed). This bounds the number of connection threads and the
/// amount of per-connection state, so that a (misbehaving or malicious)
/// local process cannot make the application allocate an arbitrary amount
/// of resources just by opening sockets. The limit is arbitrary, but
/// generous: It comfortably exceeds the number of clients the
/// application is intended to serve (dev tools, an MCP server, plugins,
/// scripts).
pub(super) const MAX_CONNECTIONS: usize = 20;

/// Maximum time [RpcServer::stop] waits for connection threads to finish.
/// They cannot be interrupted while blocked inside a dispatch callback, so
/// after this timeout they are detached with a warning (which is fine
/// during application teardown).
const STOP_TIMEOUT: Duration = Duration::from_secs(5);

/// Callback which handles a command request (fire-and-forget).
///
/// It receives the connection ID, the msg_id, the method name (e.g.
/// `"Project/list"`) and the raw msgpack params (a map, opaque to this
/// server module — its content is only interpreted by the command
/// handlers). If the callback returns `Ok(())`, the request was accepted
/// and [RpcServer::reply] will be called exactly once, at any time, from
/// any thread. Otherwise it returns the error which the connection code
/// answers with immediately (e.g. for params which could not be converted
/// at the FFI boundary).
///
/// The callback is invoked on the connection's reader thread, i.e.
/// possibly concurrently from multiple threads, but strictly sequentially
/// per connection.
///
/// Note that the `initialize` request is dispatched through this callback
/// as well: The authentication (token validation) is implemented on the
/// C++ side, and the reply to the request implicitly (un-)authenticates
/// the connection (success/error).
pub type DispatchFn =
  Arc<dyn Fn(u32, u32, &str, &Value) -> Result<(), RpcError> + Send + Sync>;

/// Callback which is invoked when a client cancels a pending request
/// (`[3, msg_id]`).
///
/// The callback must eventually make sure the request is replied exactly
/// once (typically with a "cancelled" error, or with the normal result if
/// the work already completed).
pub type CancelFn = Arc<dyn Fn(u32, u32) + Send + Sync>;

/// Internal shared state of the [RpcServer], used by all its threads.
struct Inner {
  /// The command dispatch callback (see [DispatchFn]).
  dispatch: DispatchFn,
  /// The cancel callback (see [CancelFn]).
  cancel: CancelFn,
  /// Registered connections, by connection ID.
  clients: Mutex<HashMap<u32, Connection>>,
  /// Whether the server is running (set to `false` by [RpcServer::stop]).
  running: AtomicBool,
  /// Number of currently active connections (reader threads).
  active_connections: AtomicUsize,
  /// Generator for connection IDs.
  next_conn_id: AtomicU32,
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
  /// The msg_ids of requests which were dispatched but not yet replied.
  pending: Arc<Mutex<HashSet<u32>>>,
  /// The msg_ids of dispatched `authenticate` requests which were not yet
  /// replied. The reply to such a request authenticates the connection
  /// (success) or un-authenticates it (error) — see the reply path.
  pending_authentications: Arc<Mutex<HashSet<u32>>>,
  /// Whether the connection is authenticated. Updated when a reply to an
  /// `authenticate` request is sent (see the reply path). While this is
  /// `false`, requests other than `initialize`/`authenticate` are rejected
  /// with an error instead of being dispatched.
  authenticated: Arc<AtomicBool>,
}

/// The RPC server.
///
/// Usage: Create it with [RpcServer::new], which immediately starts
/// listening on 127.0.0.1 (the port is returned via the out-parameter).
/// Send request-scoped messages with [RpcServer::notify_conn] and shut the
/// server down with [RpcServer::stop] (or just drop it, which does the
/// same).
///
/// All methods can be called from any thread. The dispatch and cancel
/// callbacks are invoked on the connection's reader threads.
pub struct RpcServer {
  /// The state shared between the server and all its threads.
  inner: Arc<Inner>,
}

impl RpcServer {
  /// Create a new server, which immediately starts listening on 127.0.0.1.
  ///
  /// The default port ([DEFAULT_PORT]) is used if possible; if it's
  /// already in use (e.g. by another LibrePCB instance), the next higher
  /// ports are tried ([MAX_PORT_ATTEMPTS] attempts). Binding a port is a
  /// cheap syscall, so the fallback logic is negligible.
  ///
  /// Returns an error only if none of the attempted ports could be bound
  /// (or if spawning the accept loop thread failed).
  pub fn new(
    dispatch: DispatchFn,
    cancel: CancelFn,
    port: &mut u16,
  ) -> io::Result<Self> {
    let mut listener = None;
    let mut last_error: Option<io::Error> = None;
    *port = DEFAULT_PORT;
    for _ in 0..MAX_PORT_ATTEMPTS {
      match TcpListener::bind(("127.0.0.1", *port)) {
        Ok(l) => {
          listener = Some(l);
          break;
        }
        // The port is already in use → try the next higher one.
        Err(e) if e.kind() == io::ErrorKind::AddrInUse => {
          last_error = Some(e);
          *port += 1;
        }
        Err(e) => return Err(e),
      }
    }
    let listener = listener.ok_or_else(|| {
      last_error.unwrap_or_else(|| {
        io::Error::new(io::ErrorKind::AddrInUse, "no free port available")
      })
    })?;
    listener.set_nonblocking(true)?;

    let inner = Arc::new(Inner {
      dispatch,
      cancel,
      clients: Mutex::new(HashMap::new()),
      running: AtomicBool::new(true),
      active_connections: AtomicUsize::new(0),
      next_conn_id: AtomicU32::new(0),
      accept_thread: Mutex::new(None),
    });
    let accept_inner = inner.clone();
    let handle = thread::Builder::new()
      .name("rpc-accept".to_string())
      .spawn(move || connection::accept_loop(accept_inner, listener))?;
    *lock(&inner.accept_thread) = Some(handle);
    Ok(Self { inner })
  }

  /// Send the response for a pending request.
  ///
  /// A second response for the same msg_id is silently dropped (returning
  /// `false`), as is a reply for an unknown or already closed connection —
  /// this enforces the "exactly one response per request" protocol
  /// invariant.
  ///
  /// This method never blocks (the message is just queued on the
  /// connection's writer thread) and may be called from any thread.
  ///
  /// Returns whether the response was sent, i.e. whether the request was
  /// actually pending.
  pub fn reply(&self, conn_id: u32, msg_id: u32, result: &Value) -> bool {
    let message = build_response(msg_id, 0, result);
    self.send_response(conn_id, msg_id, 0, &message)
  }

  /// Send an error response for a pending request: The error code is sent
  /// as the status, the error message as the payload (see
  /// [RpcError::to_payload]). See [RpcServer::reply].
  pub fn reply_error(
    &self,
    conn_id: u32,
    msg_id: u32,
    error: &RpcError,
  ) -> bool {
    let status = error.code() as i64;
    let message = build_response(msg_id, status, &error.to_payload());
    self.send_response(conn_id, msg_id, status, &message)
  }

  /// Send an error response with a raw payload, as forwarded from the C++
  /// dispatch layer: Its error map may carry additional error properties
  /// next to "code" and "message" (see the error types in the API
  /// specification), so it is forwarded as-is except that the code is sent
  /// as the status instead of being part of the payload.
  pub fn reply_error_raw(
    &self,
    conn_id: u32,
    msg_id: u32,
    status: i64,
    payload: &Value,
  ) -> bool {
    let message = build_response(msg_id, status, payload);
    self.send_response(conn_id, msg_id, status, &message)
  }

  /// Look up the connection, remove the msg_id from the pending set and
  /// queue the encoded message on its writer thread (see [RpcServer::reply]).
  ///
  /// `status` is the response status: If the replied msg_id belongs to a
  /// dispatched `authenticate` request (see
  /// [Connection::pending_authentications]), the connection is authenticated
  /// (status 0) or un-authenticated (error code). The state update happens
  /// under the same lock as queueing the response, so the ordering of
  /// concurrent `initialize` replies is preserved.
  fn send_response(
    &self,
    conn_id: u32,
    msg_id: u32,
    status: i64,
    message: &Value,
  ) -> bool {
    let conn = {
      let clients = lock(&self.inner.clients);
      clients.get(&conn_id).map(|conn| {
        (
          conn.tx.clone(),
          Arc::clone(&conn.pending),
          Arc::clone(&conn.pending_authentications),
          Arc::clone(&conn.authenticated),
        )
      })
    };
    let Some((tx, pending, pending_authentications, authenticated)) = conn
    else {
      return false;
    };
    // Removing the msg_id from the pending set must happen atomically with
    // the response being queued, so that no request message (type 2) can
    // slip in between (see notify_conn).
    let mut pending = lock(&pending);
    if !pending.remove(&msg_id) {
      return false; // Already replied (or never dispatched).
    }
    if lock(&pending_authentications).remove(&msg_id) {
      authenticated.store(status == 0, Ordering::SeqCst);
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

  /// Send a request-scoped message (message type 2, e.g. a progress report
  /// or streamed partial data) to the client which sent the request.
  ///
  /// The message is only delivered while the request is pending, i.e.
  /// before its response was sent (returns `false` otherwise). Like
  /// [RpcServer::reply], this never blocks and may be called from any
  /// thread.
  pub fn notify_conn(
    &self,
    conn_id: u32,
    msg_id: u32,
    payload: &Value,
  ) -> bool {
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
    if !pending.contains(&msg_id) {
      return false; // Not (or no longer) pending.
    }
    let message = protocol::build_request_message(msg_id, payload);
    match codec::encode_message(&message) {
      Ok(bytes) => tx.send(bytes).is_ok(),
      Err(e) => {
        eprintln!("librepcb-rust: Failed to encode a request message: {e}");
        false
      }
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
  use protocol::{METHOD_AUTHENTICATE, METHOD_INITIALIZE};
  use std::io::{BufReader, Write};
  use std::sync::mpsc;

  /// The token the test's emulated C++ initialize handler accepts.
  const TOKEN: &str = "0123456789abcdef0123456789abcdef";

  /// The name of the ping method (a normal dispatched command, emulated
  /// by the test like the C++ side does).
  const METHOD_PING: &str = "ping";

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

    /// Connect and authenticate: Send the `initialize` request, emulate
    /// the C++ handshake handler on the forwarded request (reply), then
    /// send the `authenticate` request with the test token, emulate the
    /// C++ authentication handler (validate token, mark authenticated,
    /// reply) and read the responses.
    fn new(port: u16, server: &RpcServer, dispatch: &TestDispatch) -> Self {
      let mut client = Self::new_unauthenticated(port);
      client.send_initialize();
      emulate_cpp_initialize(server, dispatch);
      assert!(client.read_ok(1).is_map(), "initialize failed");
      client.send_authenticate(TOKEN);
      assert!(
        emulate_cpp_authenticate(server, dispatch, TOKEN),
        "authenticate failed"
      );
      assert!(client.read_ok(2).is_map(), "authenticate failed");
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

    /// Send an `initialize` request (handshake, no authentication).
    fn send_initialize(&mut self) {
      self.request(
        1,
        METHOD_INITIALIZE,
        Value::Map(vec![(Value::from("protocol"), Value::from(1))]),
      );
    }

    /// Send an `authenticate` request with the given token (msgid 2).
    fn send_authenticate(&mut self, token: &str) {
      self.request(
        2,
        METHOD_AUTHENTICATE,
        Value::Map(vec![
          (Value::from("name"), Value::from("test")),
          (Value::from("token"), Value::from(token)),
        ]),
      );
    }

    /// Build a request message.
    fn request_value(msg_id: u32, method: &str, params: Value) -> Value {
      Value::Array(vec![
        Value::from(0),
        Value::from(msg_id),
        Value::from(method),
        params,
      ])
    }

    /// Send a request.
    fn request(&mut self, msg_id: u32, method: &str, params: Value) {
      self.send(&Self::request_value(msg_id, method, params));
    }

    /// Read one message (blocking, with timeout). Returns `None` on EOF,
    /// i.e. if the server closed the connection.
    fn read(&mut self) -> Option<Value> {
      codec::read_message(&mut self.reader)
        .unwrap_or_else(|e| panic!("failed to decode a message: {e}"))
    }

    /// Read a response, asserting that it is a response for the given
    /// msg_id. Returns `(status, payload)` — the status is 0 on success,
    /// otherwise the error code (the payload is then the error map).
    fn read_response(&mut self, msg_id: u32) -> (Value, Value) {
      let message = self.read().expect("connection closed unexpectedly");
      let items = message.as_array().unwrap();
      assert_eq!(items[0], Value::from(1), "not a response: {message:?}");
      assert_eq!(
        items[1],
        Value::from(msg_id),
        "unexpected msg_id: {message:?}"
      );
      assert_eq!(items.len(), 4, "invalid response: {message:?}");
      (items[2].clone(), items[3].clone())
    }

    /// Read a response, asserting that it is a success response (status
    /// 0). Returns the payload.
    fn read_ok(&mut self, msg_id: u32) -> Value {
      let (status, payload) = self.read_response(msg_id);
      assert_eq!(status, Value::from(0), "expected success: {payload:?}");
      payload
    }

    /// Read a response, asserting that it failed with the given error
    /// code (and carries an error message in the payload).
    fn read_error(&mut self, msg_id: u32, code: RpcErrorCode) {
      let (status, payload) = self.read_response(msg_id);
      assert_eq!(status, Value::from(code as i64));
      let pairs = payload.as_map().expect("expected an error payload");
      assert!(
        pairs.iter().any(|(key, _)| key.as_str() == Some("message")),
        "error payload without a message: {payload:?}"
      );
    }
  }

  /// A dispatch handler which forwards all requests into a channel (to be
  /// replied by the test code, just like the C++ integration would).
  type TestDispatch = mpsc::Receiver<(u32, u32, String, Value)>;

  /// Create a dispatch + cancel callback pair which forward the calls
  /// into channels.
  fn forwarding_handlers() -> (
    DispatchFn,
    CancelFn,
    TestDispatch,
    mpsc::Receiver<(u32, u32)>,
  ) {
    let (dispatch_tx, dispatch_rx) = mpsc::channel();
    let (cancel_tx, cancel_rx) = mpsc::channel();
    (
      Arc::new(
        move |conn_id: u32, msg_id: u32, method: &str, params: &Value| {
          dispatch_tx
            .send((conn_id, msg_id, method.to_string(), params.clone()))
            .unwrap();
          Ok(())
        },
      ),
      Arc::new(move |conn_id: u32, msg_id: u32| {
        cancel_tx.send((conn_id, msg_id)).unwrap();
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
    let mut port = 0;
    let server = RpcServer::new(dispatch, cancel, &mut port).unwrap();
    (server, port)
  }

  /// Create a started test server with forwarding handlers.
  fn spawn_server() -> (RpcServer, u16, TestDispatch, mpsc::Receiver<(u32, u32)>)
  {
    let (dispatch, cancel, dispatch_rx, cancel_rx) = forwarding_handlers();
    let (server, port) = spawn_server_with_handlers(dispatch, cancel);
    (server, port, dispatch_rx, cancel_rx)
  }

  /// Emulate the C++ handshake handler on a forwarded `initialize`
  /// request: Reply with the negotiated version (does not authenticate).
  fn emulate_cpp_initialize(server: &RpcServer, dispatch: &TestDispatch) {
    let (conn_id, msg_id, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_INITIALIZE);
    assert_eq!(field(&params, "protocol"), Some(&Value::from(1)));
    server.reply(
      conn_id,
      msg_id,
      &Value::Map(vec![(Value::from("protocol"), Value::from(1))]),
    );
  }

  /// Emulate the C++ authentication handler on a forwarded `authenticate`
  /// request: Validate the token and reply. The server authenticates the
  /// connection implicitly from the reply (success/error).
  /// Returns whether the token was accepted.
  fn emulate_cpp_authenticate(
    server: &RpcServer,
    dispatch: &TestDispatch,
    expected_token: &str,
  ) -> bool {
    let (conn_id, msg_id, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_AUTHENTICATE);
    let token = params
      .as_map()
      .unwrap()
      .iter()
      .find(|(k, _)| k.as_str() == Some("token"))
      .map(|(_, v)| v.as_str().unwrap().to_string());
    let authenticated = token.as_deref() == Some(expected_token);
    if authenticated {
      server.reply(conn_id, msg_id, &Value::Map(Vec::new()));
    } else {
      server.reply_error(
        conn_id,
        msg_id,
        &RpcError::canonical(RpcErrorCode::Unauthenticated),
      );
    }
    authenticated
  }

  /// Emulate the C++ ping handler on a forwarded request (reply nil).
  fn emulate_cpp_ping(server: &RpcServer, dispatch: &TestDispatch) {
    let (conn_id, msg_id, method, _params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_PING);
    server.reply(conn_id, msg_id, &Value::Nil);
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
  fn test_initialize_and_authenticate() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    // The handshake is dispatched to the handler, with the format in the
    // params. Its reply does NOT authenticate the connection:
    client.send_initialize();
    let (conn_id, msg_id, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_INITIALIZE);
    assert_eq!(field(&params, "protocol"), Some(&Value::from(1)));
    server.reply(
      conn_id,
      msg_id,
      &Value::Map(vec![(Value::from("protocol"), Value::from(1))]),
    );
    assert!(client.read_ok(1).is_map());
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(2, RpcErrorCode::Unauthenticated);
    // The authentication is dispatched with the token in the params (the
    // whole validation is up to the C++ side) — its reply authenticates:
    client.send_authenticate(TOKEN);
    let (conn_id, msg_id, method, params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(method, METHOD_AUTHENTICATE);
    assert_eq!(field(&params, "token"), Some(&Value::from(TOKEN)));
    server.reply(conn_id, msg_id, &Value::Map(Vec::new()));
    assert!(client.read_ok(2).is_map());
    // The connection is now usable:
    client.request(3, METHOD_PING, Value::Map(Vec::new()));
    emulate_cpp_ping(&server, &dispatch);
    assert!(client.read_ok(3).is_nil());
  }

  #[test]
  fn test_authenticate_with_wrong_token() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize();
    emulate_cpp_initialize(&server, &dispatch);
    assert!(client.read_ok(1).is_map());
    // The C++ side rejects the wrong token. The connection stays open,
    // but any other request is rejected:
    client.send_authenticate("wrong");
    assert!(!emulate_cpp_authenticate(&server, &dispatch, TOKEN));
    client.read_error(2, RpcErrorCode::Unauthenticated);
    client.request(3, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(3, RpcErrorCode::Unauthenticated);
  }

  #[test]
  fn test_authenticate_with_wrong_token_after_auth() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize();
    emulate_cpp_initialize(&server, &dispatch);
    assert!(client.read_ok(1).is_map());
    client.send_authenticate(TOKEN);
    assert!(emulate_cpp_authenticate(&server, &dispatch, TOKEN));
    assert!(client.read_ok(2).is_map());
    // Re-authenticate with a wrong token → the C++ side un-authenticates
    // the connection and replies with an error. The connection stays
    // open, but further requests are rejected:
    client.send_authenticate("wrong");
    assert!(!emulate_cpp_authenticate(&server, &dispatch, TOKEN));
    client.read_error(2, RpcErrorCode::Unauthenticated);
    client.request(3, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(3, RpcErrorCode::Unauthenticated);
    // However, authenticating correctly again works:
    client.send_authenticate(TOKEN);
    assert!(emulate_cpp_authenticate(&server, &dispatch, TOKEN));
    assert!(client.read_ok(2).is_map());
  }

  #[test]
  fn test_first_request_not_initialize() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new_unauthenticated(port);
    // A request other than initialize/authenticate on an unauthenticated
    // connection is rejected without being dispatched, and the connection
    // stays open:
    client.request(1, METHOD_PING, Value::Map(Vec::new()));
    client.read_error(1, RpcErrorCode::Unauthenticated);
    assert!(dispatch.try_recv().is_err());
    // Handshaking and authenticating afterwards works:
    client.send_initialize();
    emulate_cpp_initialize(&server, &dispatch);
    assert!(client.read_ok(1).is_map());
    client.send_authenticate(TOKEN);
    assert!(emulate_cpp_authenticate(&server, &dispatch, TOKEN));
    assert!(client.read_ok(2).is_map());
  }

  #[test]
  fn test_ping() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(5, METHOD_PING, Value::Map(Vec::new()));
    emulate_cpp_ping(&server, &dispatch);
    assert!(client.read_ok(5).is_nil());
  }

  #[test]
  fn test_dispatch_and_reply() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    let params = Value::Map(vec![
      (Value::from("obj"), Value::from("some-uuid")),
      (Value::from("x"), Value::from(1)),
    ]);
    client.request(42, "Project/get", params.clone());
    // The dispatch handler received the request...
    let (conn_id, msg_id, method, received_params) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!(msg_id, 42);
    assert_eq!(method, "Project/get");
    assert_eq!(received_params, params);
    // ...and we reply asynchronously, just like the C++ integration does:
    let result = server.reply(
      conn_id,
      msg_id,
      &Value::Map(vec![(Value::from("name"), Value::from("Test"))]),
    );
    assert!(result);
    assert_eq!(
      field(&client.read_ok(42), "name"),
      Some(&Value::from("Test"))
    );
  }

  #[test]
  fn test_dispatch_immediate_error() {
    // A handler which rejects every request with an error (e.g. params
    // conversion failures at the FFI boundary). Since `initialize` is
    // rejected as well, the connection can never be authenticated — which
    // is fine for this test.
    let dispatch: DispatchFn = Arc::new(|_, _, _, _| {
      Err(RpcError::canonical(RpcErrorCode::InvalidArgs))
    });
    let cancel: CancelFn = Arc::new(|_, _| {});
    let (_server, port) = spawn_server_with_handlers(dispatch, cancel);
    let mut client = TestClient::new_unauthenticated(port);
    client.send_initialize();
    client.read_error(1, RpcErrorCode::InvalidArgs);
  }

  #[test]
  fn test_exactly_one_reply() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // The first reply is sent, a second one for the same msg_id is dropped:
    assert!(server.reply(conn_id, msg_id, &Value::Nil));
    assert!(!server.reply(conn_id, msg_id, &Value::Nil));
    assert!(client.read_ok(1).is_nil());
    // No second response arrives, i.e. the next message is the ping
    // response:
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    emulate_cpp_ping(&server, &dispatch);
    client.read_response(2);
  }

  #[test]
  fn test_reply_for_unknown_msg_id() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // A reply for a msg_id which was never dispatched:
    assert!(!server.reply(u32::MAX, 123, &Value::Nil));
    // A reply for a valid connection, but an unknown msg_id:
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    assert!(!server.reply(conn_id, msg_id + 100, &Value::Nil));
    // The pending request is still pending, i.e. it can be replied:
    assert!(server.reply(conn_id, msg_id, &Value::Nil));
    assert!(client.read_ok(1).is_nil());
  }

  #[test]
  fn test_reply_after_connection_closed() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    drop(client); // Close the connection.
                  // Give the server a moment to process the disconnect.
    thread::sleep(Duration::from_millis(100));
    // The reply is silently dropped instead of panicking:
    assert!(!server.reply(conn_id, msg_id, &Value::Nil));
  }

  #[test]
  fn test_notify_conn_only_while_pending() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // While the request is pending, request messages (type 2) arrive:
    let progress = Value::Map(vec![
      (Value::from("percent"), Value::from(50)),
      (Value::from("message"), Value::from("Working")),
    ]);
    assert!(server.notify_conn(conn_id, msg_id, &progress));
    let message = client.read().unwrap();
    let items = message.as_array().unwrap();
    assert_eq!(items[0], Value::from(2));
    assert_eq!(items[1], Value::from(1));
    assert_eq!(field(&items[2], "percent"), Some(&Value::from(50)));
    // After the response was sent, no further messages are accepted:
    assert!(server.reply(conn_id, msg_id, &Value::Nil));
    assert!(!server.notify_conn(conn_id, msg_id, &progress));
    client.read_response(1);
  }

  #[test]
  fn test_cancel_is_forwarded() {
    let (server, port, dispatch, cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // Cancel the pending request → the cancel handler is invoked...
    client.send(&Value::Array(vec![Value::from(3), Value::from(1)]));
    let (cancelled_conn, cancelled_msg_id) =
      cancel.recv_timeout(Duration::from_secs(10)).unwrap();
    assert_eq!((cancelled_conn, cancelled_msg_id), (conn_id, msg_id));
    // ...and we reply with a "cancelled" error, as the C++ side would:
    assert!(server.reply_error(
      conn_id,
      msg_id,
      &RpcError::new(RpcErrorCode::Cancelled, "cancelled")
    ));
    client.read_error(1, RpcErrorCode::Cancelled);
  }

  #[test]
  fn test_cancel_for_unknown_msg_id_is_ignored() {
    let (server, port, dispatch, cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // Cancel a msg_id which was never dispatched: ignored (no cancel
    // callback, no response) — the next message is the ping response.
    client.send(&Value::Array(vec![Value::from(3), Value::from(123)]));
    client.request(2, METHOD_PING, Value::Map(Vec::new()));
    emulate_cpp_ping(&server, &dispatch);
    client.read_response(2);
    assert!(cancel.try_recv().is_err());
  }

  #[test]
  fn test_pipelining_requests() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    // Send three requests without waiting for responses. Since dispatch
    // is asynchronous, all of them are dispatched before any reply:
    for msg_id in 1..=3u32 {
      client.request(msg_id, "Project/list", Value::Map(Vec::new()));
    }
    let mut requests = Vec::new();
    for _ in 0..3 {
      requests.push(dispatch.recv_timeout(Duration::from_secs(10)).unwrap());
    }
    // Reply in reverse order — responses may arrive in any order:
    for (conn_id, msg_id, _, _) in requests.iter().rev() {
      assert!(server.reply(*conn_id, *msg_id, &Value::from(*msg_id)));
    }
    for msg_id in (1..=3u32).rev() {
      assert_eq!(client.read_ok(msg_id), Value::from(msg_id));
    }
  }

  #[test]
  fn test_duplicate_msg_id_is_rejected() {
    let (server, port, dispatch, _cancel) = spawn_server();
    let mut client = TestClient::new(port, &server, &dispatch);
    client.request(1, "Project/list", Value::Map(Vec::new()));
    let (conn_id, msg_id, _, _) =
      dispatch.recv_timeout(Duration::from_secs(10)).unwrap();
    // A second request with the same msg_id while the first one is still
    // pending is a protocol violation and answered with an error, without
    // being dispatched:
    client.request(1, "Project/list", Value::Map(Vec::new()));
    client.read_error(1, RpcErrorCode::InvalidRequest);
    // The first request is still pending and can be replied:
    assert!(server.reply(conn_id, msg_id, &Value::Nil));
    assert!(client.read_ok(1).is_nil());
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
  fn test_two_servers_get_different_ports() {
    let (server1, port1, _dispatch1, _cancel1) = spawn_server();
    let (server2, port2, _dispatch2, _cancel2) = spawn_server();
    // The second server can't bind the port of the first one, so the
    // fallback logic assigns a different port.
    assert_ne!(port1, port2);
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

  /// Connections beyond the limit are rejected: The server closes the
  /// socket immediately, without any protocol-level message (the
  /// connection was not even authenticated, so there is no reply to
  /// negotiate an error response with).
  #[test]
  fn test_connection_limit() {
    let (server, port, _dispatch, _cancel) = spawn_server();
    // Open clients up to the limit. Note that the accept loop polls, so
    // wait until all of them were actually accepted before going beyond
    // the limit.
    let mut clients = Vec::new();
    for _ in 0..MAX_CONNECTIONS {
      clients.push(TestClient::new_unauthenticated(port));
    }
    let deadline = std::time::Instant::now() + Duration::from_secs(5);
    while server.inner.active_connections.load(Ordering::SeqCst)
      < MAX_CONNECTIONS
    {
      assert!(
        std::time::Instant::now() < deadline,
        "connections were not accepted in time"
      );
      thread::sleep(Duration::from_millis(10));
    }
    // The next connection is closed by the server right away, i.e. the
    // client reads EOF without ever receiving a message.
    let mut rejected = TestClient::new_unauthenticated(port);
    assert!(rejected.read().is_none());
  }
}
