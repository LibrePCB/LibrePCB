//! Connection handling for the RPC server: The accept loop and the
//! per-connection reader/writer threads.

use std::io::{self, BufReader, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::Ordering;
use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use rmpv::Value;

use super::codec::{self, read_message};
use super::protocol::{
  self, build_response, classify_message, error_code, extract_token,
  ParsedMessage, RpcError, METHOD_INITIALIZE, METHOD_PING,
};
use super::{lock, Connection, DispatchOutcome, Inner};

/// Poll interval of the non-blocking accept loop.
const ACCEPT_POLL_INTERVAL: Duration = Duration::from_millis(10);

/// The accept loop, running on its own thread as long as the server runs.
pub(super) fn accept_loop(inner: Arc<Inner>, listener: TcpListener) {
  loop {
    if !inner.running.load(Ordering::SeqCst) {
      break;
    }
    match listener.accept() {
      Ok((stream, _addr)) => {
        // Note: The counter is incremented *before* spawning, to avoid an
        // underflow in case the thread finishes before the increment.
        inner.active_connections.fetch_add(1, Ordering::SeqCst);
        let conn_id = inner.next_conn_id.fetch_add(1, Ordering::SeqCst);
        let inner_clone = inner.clone();
        let spawn_result = thread::Builder::new()
          .name(format!("rpc-conn-{conn_id}"))
          .spawn(move || handle_connection(inner_clone, stream, conn_id));
        if spawn_result.is_err() {
          eprintln!("librepcb-rust: Spawning connection thread failed");
          inner.active_connections.fetch_sub(1, Ordering::SeqCst);
        }
      }
      Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
        thread::sleep(ACCEPT_POLL_INTERVAL);
      }
      Err(e) => {
        eprintln!("librepcb-rust: Accepting connection failed: {e}");
        break;
      }
    }
  }
}

/// Handle one client connection: Set up the writer thread, then run the
/// reader loop until the connection is closed. Runs on the connection's
/// reader thread.
fn handle_connection(inner: Arc<Inner>, stream: TcpStream, conn_id: u64) {
  // If the server was stopped in the moment this connection was accepted,
  // drop the connection right away.
  if !inner.running.load(Ordering::SeqCst) {
    inner.active_connections.fetch_sub(1, Ordering::SeqCst);
    return;
  }
  // The listener is non-blocking, and this state may be inherited by
  // accepted sockets on some platforms, so ensure blocking halves.
  let read_half = match stream_clone_blocking(&stream) {
    Ok(stream) => stream,
    Err(e) => {
      eprintln!("librepcb-rust: Cloning connection stream failed: {e}");
      inner.active_connections.fetch_sub(1, Ordering::SeqCst);
      return;
    }
  };
  let write_half = match stream_clone_blocking(&stream) {
    Ok(stream) => stream,
    Err(e) => {
      eprintln!("librepcb-rust: Cloning connection stream failed: {e}");
      inner.active_connections.fetch_sub(1, Ordering::SeqCst);
      return;
    }
  };
  // Register the connection. The registry keeps a clone of the sender
  // until this thread removes the entry, which (together with dropping
  // our own clone below) closes the writer thread's channel.
  let (tx, rx) = mpsc::channel::<Vec<u8>>();
  let pending = Arc::new(Mutex::new(std::collections::HashSet::new()));
  lock(&inner.clients).insert(
    conn_id,
    Connection {
      tx: tx.clone(),
      stream,
      pending: Arc::clone(&pending),
    },
  );
  let writer_thread = match thread::Builder::new()
    .name(format!("rpc-conn-{conn_id}-writer"))
    .spawn(move || {
      let mut write_half = write_half;
      while let Ok(bytes) = rx.recv() {
        if write_half.write_all(&bytes).is_err() {
          break;
        }
      }
    }) {
    Ok(handle) => handle,
    Err(e) => {
      eprintln!("librepcb-rust: Spawning writer thread failed: {e}");
      lock(&inner.clients).remove(&conn_id);
      inner.active_connections.fetch_sub(1, Ordering::SeqCst);
      return;
    }
  };
  let mut authenticated = false;
  let mut reader = BufReader::new(read_half);
  loop {
    match read_message(&mut reader) {
      Ok(None) => break, // Clean EOF between messages.
      Ok(Some(message)) => {
        let (response, close) = process_message(
          &inner,
          conn_id,
          &pending,
          &message,
          &mut authenticated,
        );
        if let Some(response) = response {
          if let Ok(bytes) = codec::encode_message(&response) {
            if tx.send(bytes).is_err() {
              break; // The writer thread is gone, so the connection is dead.
            }
          }
        }
        if close {
          break;
        }
      }
      Err(e) => {
        // The stream framing is desynchronized (invalid msgpack, a
        // truncated message or an oversized message), so the connection
        // must be dropped instead of trying to recover.
        eprintln!("librepcb-rust: Dropping broken connection: {e}");
        break;
      }
    }
  }
  // Unregister the connection and let the writer thread finish (its
  // channel gets closed by dropping our sender clone and the registry
  // entry). Pending messages are still drained before it exits.
  lock(&inner.clients).remove(&conn_id);
  drop(tx);
  let _ = writer_thread.join();
  inner.active_connections.fetch_sub(1, Ordering::SeqCst);
}

/// Process one incoming message.
///
/// Returns the response (if any) and whether the connection must be
/// closed afterwards. The response is returned as a [Value] and encoded
/// by the caller — the writer thread only deals with bytes.
fn process_message(
  inner: &Inner,
  conn_id: u64,
  pending: &Arc<Mutex<std::collections::HashSet<u32>>>,
  message: &Value,
  authenticated: &mut bool,
) -> (Option<Value>, bool) {
  match classify_message(message) {
    ParsedMessage::Request {
      msgid,
      method,
      params,
    } => {
      if method == METHOD_INITIALIZE {
        let (response, token_ok) = handle_initialize(inner, &params, msgid);
        if token_ok {
          *authenticated = true;
        }
        // A wrong token always drops the connection, even if it was
        // previously authenticated (this looks like a hijacking attempt).
        (Some(response), !token_ok)
      } else if !*authenticated {
        // Before authentication, any request other than `initialize` is
        // rejected and the connection is dropped.
        let error = RpcError::new(
          error_code::UNAUTHENTICATED,
          "This connection is not authenticated, the first message must be \
           an 'initialize' request",
        );
        (
          Some(build_response(msgid, Some(&error.to_value()), &Value::Nil)),
          true,
        )
      } else if method == METHOD_PING {
        (
          Some(build_response(msgid, None, &Value::Map(Vec::new()))),
          false,
        )
      } else {
        (
          dispatch_request(inner, conn_id, pending, msgid, method, params),
          false,
        )
      }
    }
    ParsedMessage::Cancel { msgid } => {
      // The cancel handler is only invoked for pending requests, i.e.
      // ones which were dispatched but not yet replied. It is responsible
      // for eventually sending exactly one response (typically a
      // "cancelled" error).
      if lock(pending).contains(&msgid) {
        (inner.config.cancel)(conn_id, msgid);
      } else {
        eprintln!("librepcb-rust: Ignoring cancel for unknown msgid {msgid}");
      }
      (None, false)
    }
    ParsedMessage::Ignored => {
      // Wrong direction or unknown message type: ignored for forward
      // compatibility. Before authentication, the connection is dropped.
      (None, !*authenticated)
    }
    ParsedMessage::Invalid { msgid } => {
      // A request which doesn't conform to the envelope: answered with an
      // error (the stream is still in sync). Before authentication, the
      // connection is dropped.
      let response = build_response(
        msgid.unwrap_or(0),
        Some(&RpcError::invalid_request().to_value()),
        &Value::Nil,
      );
      (Some(response), !*authenticated)
    }
  }
}

/// Handle an `initialize` request: Check the token and build the response.
///
/// Returns the response and whether the token was valid.
fn handle_initialize(
  inner: &Inner,
  params: &Value,
  msgid: u32,
) -> (Value, bool) {
  match extract_token(params) {
    Some(token) if token == inner.config.token => (
      build_response(
        msgid,
        None,
        &protocol::build_initialize_result(
          &inner.config.info,
          &inner.config.protocol_version,
        ),
      ),
      true,
    ),
    _ => {
      let error =
        RpcError::new(error_code::UNAUTHENTICATED, "Invalid or missing token");
      (
        build_response(msgid, Some(&error.to_value()), &Value::Nil),
        false,
      )
    }
  }
}

/// Handle a request by handing it over to the dispatch callback.
///
/// The request's msgid is marked as pending *before* the callback is
/// invoked (so that a synchronous reply from within the callback works),
/// and the callback decides how to proceed:
/// [DispatchOutcome::Dispatched] means the response will be sent
/// asynchronously, [DispatchOutcome::ReplyNow] answers immediately.
fn dispatch_request(
  inner: &Inner,
  conn_id: u64,
  pending: &Arc<Mutex<std::collections::HashSet<u32>>>,
  msgid: u32,
  method: String,
  params: Value,
) -> Option<Value> {
  if !lock(pending).insert(msgid) {
    // A msgid must not be reused while the request is still pending.
    let error = RpcError::new(
      error_code::INVALID_REQUEST,
      "The msgid is already in use by a pending request",
    );
    return Some(build_response(msgid, Some(&error.to_value()), &Value::Nil));
  }
  match (inner.config.dispatch)(conn_id, msgid, &method, &params) {
    DispatchOutcome::Dispatched => None,
    DispatchOutcome::ReplyNow(error) => {
      lock(pending).remove(&msgid);
      Some(build_response(msgid, Some(&error.to_value()), &Value::Nil))
    }
  }
}

/// Clone a TCP stream and make the clone blocking (the listener is
/// non-blocking, and this state may be inherited by accepted sockets on
/// some platforms).
fn stream_clone_blocking(stream: &TcpStream) -> io::Result<TcpStream> {
  let clone = stream.try_clone()?;
  clone.set_nonblocking(false)?;
  Ok(clone)
}
