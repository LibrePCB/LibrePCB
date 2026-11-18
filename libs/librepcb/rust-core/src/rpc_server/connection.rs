//! Connection handling for the TCP RPC server: The accept loop and the
//! per-connection reader/writer threads.

use std::io::{self, BufReader};
use std::net::{TcpListener, TcpStream};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::atomic::Ordering;
use std::sync::mpsc;
use std::sync::Arc;
use std::thread;
use std::time::Duration;

use serde_json::{Map, Value};

use super::codec::{read_message, write_message};
use super::protocol::{
  build_error_response, build_result_response, classify_message, error_code, ParsedMessage,
  RpcError, METHOD_INITIALIZE, METHOD_PING, METHOD_RPC_DISCOVER,
};
use super::{lock, Connection, Inner};

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
  let (tx, rx) = mpsc::channel::<String>();
  lock(&inner.clients).insert(
    conn_id,
    Connection {
      tx: tx.clone(),
      stream,
    },
  );
  let writer_thread = match thread::Builder::new()
    .name(format!("rpc-conn-{conn_id}-writer"))
    .spawn(move || {
      let mut write_half = write_half;
      while let Ok(message) = rx.recv() {
        if write_message(&mut write_half, &message).is_err() {
          break;
        }
      }
    })
  {
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
      Ok(None) => break, // Clean EOF.
      Ok(Some(message)) => {
        let (response, close) = process_message(&inner, &message, &mut authenticated);
        if let Some(response) = response {
          if tx.send(response).is_err() {
            break; // The writer thread is gone, so the connection is dead.
          }
        }
        if close {
          break;
        }
      }
      Err(e) => {
        // The framing is broken (oversized or non-UTF-8 message). Drop
        // the connection instead of trying to recover.
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
/// Returns the serialized response (if any) and whether the connection must
/// be closed afterwards.
fn process_message(
  inner: &Inner,
  message: &str,
  authenticated: &mut bool,
) -> (Option<String>, bool) {
  let value = match serde_json::from_str::<Value>(message) {
    Ok(value) => value,
    Err(_) => {
      // Not valid JSON at all, so the request ID is unknown. Per
      // specification, the error response then must use the ID `null`.
      let response = build_error_response(Value::Null, &RpcError::parse_error());
      // An unauthenticated connection is dropped immediately.
      return (Some(response), !*authenticated);
    }
  };
  match classify_message(&value) {
    ParsedMessage::Request { id, method, params } => {
      if method == METHOD_INITIALIZE {
        let (response, token_ok) = handle_initialize(inner, &params, id);
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
        (Some(build_error_response(id, &error)), true)
      } else if method == METHOD_PING {
        (Some(build_result_response(id, Value::Object(Map::new()))), false)
      } else if method == METHOD_RPC_DISCOVER {
        (Some(handle_rpc_discover(inner, id)), false)
      } else {
        (Some(dispatch_request(inner, id, &method, params)), false)
      }
    }
    ParsedMessage::Notification => {
      // The protocol does not define any client->server notifications,
      // so they are ignored (per JSON-RPC, no response must be sent).
      (None, !*authenticated)
    }
    ParsedMessage::Response => {
      // A client must never send a response, so it is just ignored.
      (None, !*authenticated)
    }
    ParsedMessage::Invalid { id } => {
      // Per specification, the ID is `null` if it could not be detected.
      let response =
        build_error_response(id.unwrap_or(Value::Null), &RpcError::invalid_request());
      // After authentication, the connection is kept open. Before, it is
      // dropped.
      (Some(response), !*authenticated)
    }
  }
}

/// Handle an `initialize` request: Check the token and build the response.
///
/// Returns the serialized response and whether the token was valid.
fn handle_initialize(inner: &Inner, params: &Option<Value>, id: Value) -> (String, bool) {
  let token = match params {
    Some(Value::Object(map)) => map.get("token").and_then(Value::as_str),
    _ => None,
  };
  match token {
    Some(token) if token == inner.config.token => {
      (build_result_response(id, build_initialize_result(inner)), true)
    }
    _ => {
      let error = RpcError::new(error_code::UNAUTHENTICATED, "Invalid or missing token");
      (build_error_response(id, &error), false)
    }
  }
}

/// Build the `initialize` result: The configured info, plus the protocol
/// version.
fn build_initialize_result(inner: &Inner) -> Value {
  let mut map = match &inner.config.info {
    Value::Object(map) => map.clone(),
    _ => Map::new(), // Not an object: Ignore it (should never happen).
  };
  map.insert(
    "protocolVersion".to_string(),
    Value::String(inner.config.protocol_version.clone()),
  );
  Value::Object(map)
}

/// Handle the `rpc.discover` request (returns the API schema, if set).
fn handle_rpc_discover(inner: &Inner, id: Value) -> String {
  let schema = lock(&inner.api_schema).clone();
  match schema {
    Some(schema) => match serde_json::from_str::<Value>(&schema) {
      Ok(value) => build_result_response(id, value),
      Err(e) => {
        eprintln!("librepcb-rust: The API schema is not valid JSON: {e}");
        let error = RpcError::internal_error().with_data(Value::from("invalid API schema"));
        build_error_response(id, &error)
      }
    },
    None => {
      let error = RpcError::new(error_code::METHOD_NOT_FOUND, "API schema not available");
      build_error_response(id, &error)
    }
  }
}

/// Handle a request by forwarding it to the dispatch callback.
fn dispatch_request(inner: &Inner, id: Value, method: &str, params: Option<Value>) -> String {
  if !inner.running.load(Ordering::SeqCst) {
    let error = RpcError::new(error_code::SERVER_SHUTTING_DOWN, "The server is shutting down");
    return build_error_response(id, &error);
  }
  // The dispatch callback receives the params as raw JSON. Forwarding the
  // original substring of the received message would be more efficient,
  // but serializing the (already parsed) value is much simpler and cheap
  // compared to any command execution.
  let params = params.map(|p| p.to_string()).unwrap_or("null".to_string());
  // A panicking callback (e.g. a C++ integration bug) must not kill the
  // connection thread.
  let result = catch_unwind(AssertUnwindSafe(|| (inner.config.dispatch)(method, &params)))
    .unwrap_or_else(|_| {
      Err(RpcError::new(
        error_code::INTERNAL_ERROR,
        "Command handler panicked",
      ))
    });
  match result {
    Ok(result) => match serde_json::from_str::<Value>(&result) {
      Ok(value) => build_result_response(id, value),
      Err(e) => {
        eprintln!("librepcb-rust: Command '{method}' returned invalid JSON: {e}");
        let error = RpcError::new(
          error_code::INTERNAL_ERROR,
          "Command handler returned invalid JSON",
        );
        build_error_response(id, &error)
      }
    },
    Err(error) => build_error_response(id, &error),
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
