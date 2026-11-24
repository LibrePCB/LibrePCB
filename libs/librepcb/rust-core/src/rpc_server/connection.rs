//! Connection handling for the RPC server: The accept loop and the
//! per-connection reader/writer threads.

use std::io::{self, BufReader, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use rmpv::Value;

use super::codec::{self, read_message};
use super::protocol::{
  build_response, classify_message, ParsedMessage, RpcError, METHOD_INITIALIZE,
};
use super::{lock, Connection, Inner, RpcErrorCode};

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
fn handle_connection(inner: Arc<Inner>, stream: TcpStream, conn_id: u32) {
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
  let pending_initializes =
    Arc::new(Mutex::new(std::collections::HashSet::new()));
  let authenticated = Arc::new(AtomicBool::new(false));
  lock(&inner.clients).insert(
    conn_id,
    Connection {
      tx: tx.clone(),
      stream,
      pending: Arc::clone(&pending),
      pending_initializes: Arc::clone(&pending_initializes),
      authenticated: Arc::clone(&authenticated),
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
  let mut reader = BufReader::new(read_half);
  loop {
    match read_message(&mut reader) {
      Ok(None) => break, // Clean EOF between messages.
      Ok(Some(message)) => {
        // Note: Connections are never closed because of a received message
        // — only decode errors (framing is broken) close the connection.
        if let Some(response) = process_message(
          &inner,
          conn_id,
          &pending,
          &pending_initializes,
          &authenticated,
          &message,
        ) {
          if let Ok(bytes) = codec::encode_message(&response) {
            if tx.send(bytes).is_err() {
              break; // The writer thread is gone, so the connection is dead.
            }
          }
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
/// Returns the response (if any), which is encoded and sent by the
/// caller — the writer thread only deals with bytes.
fn process_message(
  inner: &Inner,
  conn_id: u32,
  pending: &Arc<Mutex<std::collections::HashSet<u32>>>,
  pending_initializes: &Arc<Mutex<std::collections::HashSet<u32>>>,
  authenticated: &Arc<AtomicBool>,
  message: &Value,
) -> Option<Value> {
  match classify_message(message) {
    ParsedMessage::Request {
      msgid,
      method,
      params,
    } => {
      // The `initialize` request is always dispatched (that's how a
      // connection gets authenticated — the token validation and the
      // reply live on the C++ side, which authenticates the connection
      // implicitly by its reply). All other requests are only dispatched
      // while the connection is authenticated.
      if (method != METHOD_INITIALIZE)
        && (!authenticated.load(Ordering::SeqCst))
      {
        let error = RpcError::canonical(RpcErrorCode::Unauthenticated);
        return Some(build_response(
          msgid,
          RpcErrorCode::Unauthenticated as i64,
          &error.to_payload(),
        ));
      }
      dispatch_request(
        inner,
        conn_id,
        pending,
        pending_initializes,
        msgid,
        method,
        params,
      )
    }
    ParsedMessage::Cancel { msgid } => {
      // The cancel handler is only invoked for pending requests, i.e.
      // ones which were dispatched but not yet replied. It is responsible
      // for eventually sending exactly one response (typically a
      // "cancelled" error).
      if lock(pending).contains(&msgid) {
        (inner.cancel)(conn_id, msgid);
      } else {
        eprintln!("librepcb-rust: Ignoring cancel for unknown msgid {msgid}");
      }
      None
    }
    ParsedMessage::Ignored => {
      // Wrong direction or unknown message type: ignored for forward
      // compatibility (the stream is still in sync).
      None
    }
    ParsedMessage::Invalid { msgid } => {
      // A request which doesn't conform to the envelope: answered with an
      // error (the stream is still in sync).
      let error = RpcError::canonical(RpcErrorCode::InvalidRequest);
      Some(build_response(
        msgid.unwrap_or(0),
        RpcErrorCode::InvalidRequest as i64,
        &error.to_payload(),
      ))
    }
  }
}

/// Handle a request by handing it over to the dispatch callback.
///
/// The request's msgid is marked as pending *before* the callback is
/// invoked (so that a synchronous reply from within the callback works).
/// If the callback returns `Ok(())`, the response will be sent
/// asynchronously; otherwise the returned error is answered immediately.
fn dispatch_request(
  inner: &Inner,
  conn_id: u32,
  pending: &Arc<Mutex<std::collections::HashSet<u32>>>,
  pending_initializes: &Arc<Mutex<std::collections::HashSet<u32>>>,
  msgid: u32,
  method: String,
  params: Value,
) -> Option<Value> {
  if !lock(pending).insert(msgid) {
    // A msgid must not be reused while the request is still pending.
    let error = RpcError::new(
      RpcErrorCode::InvalidRequest,
      "The msgid is already in use by a pending request",
    );
    return Some(build_response(
      msgid,
      error.code() as i64,
      &error.to_payload(),
    ));
  }
  // The reply to an `initialize` request authenticates the connection
  // (see the reply path in the server), so the msgid must be known there.
  if method == METHOD_INITIALIZE {
    lock(pending_initializes).insert(msgid);
  }
  match (inner.dispatch)(conn_id, msgid, &method, &params) {
    Ok(()) => None,
    Err(error) => {
      lock(pending).remove(&msgid);
      lock(pending_initializes).remove(&msgid);
      Some(build_response(
        msgid,
        error.code() as i64,
        &error.to_payload(),
      ))
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
