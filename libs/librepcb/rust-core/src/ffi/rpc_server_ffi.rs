//! FFI for [rpc_server], to be used from C++ through cbindgen.

use super::cpp_ffi::*;
use crate::rpc_server::{
  error_code, DispatchFn, RpcError, RpcServer, RpcServerConfig,
};
use std::ffi::c_void;
use std::sync::Arc;

use serde_json::Value;

/// The C++ dispatch callback (function pointer + context).
///
/// <div class="warning">
/// A raw function pointer is not [Send]/[Sync] if its signature contains
/// raw pointers, so we wrap both the callback and its context in a newtype.
/// The C++ side guarantees that they stay valid & synchronized for the
/// whole lifetime of the server, thus it is safe to share them between
/// the connection threads.
/// </div>
#[derive(Clone, Copy)]
struct CppDispatch {
  /// The callback function.
  f: RpcDispatchFn,
  /// The context pointer passed to the callback.
  ctx: *mut c_void,
}

// SAFETY: See the warning on [CppDispatch].
unsafe impl Send for CppDispatch {}
// SAFETY: See the warning on [CppDispatch].
unsafe impl Sync for CppDispatch {}

/// Call the C++ dispatch callback with the given method and params.
///
/// This is a separate function (instead of inlining it into the closure
/// below) on purpose: Rust's disjoint closure captures would otherwise
/// capture the individual (raw) fields of [CppDispatch], which would
/// make the closure not [Send]/[Sync] although the struct itself is.
fn call_dispatch(
  callback: CppDispatch,
  method: &str,
  params: &str,
) -> Result<String, RpcError> {
  unsafe {
    let method_q = qstring_new();
    assert!(!method_q.is_null());
    qstring_set(&mut *method_q, method);
    let params_q = qstring_new();
    assert!(!params_q.is_null());
    qstring_set(&mut *params_q, params);
    let response_q = qstring_new();
    assert!(!response_q.is_null());
    let ok =
      (callback.f)(callback.ctx, &*method_q, &*params_q, &mut *response_q);
    let response = from_qstring(&*response_q);
    qstring_delete(method_q);
    qstring_delete(params_q);
    qstring_delete(response_q);
    if ok {
      Ok(response)
    } else {
      parse_error_response(&response)
    }
  }
}

/// Callback which dispatches a command request from the Rust server to C++.
///
/// It is invoked on the connection's reader thread, i.e. possibly
/// concurrently from multiple threads, but strictly sequentially per
/// connection. The C++ implementation is responsible for marshalling the
/// command to the GUI thread (it must never block or deadlock it!).
///
/// - `ctx`: The context pointer passed to [ffi_rpc_server_new].
/// - `method`: The method name, e.g. "project/list".
/// - `params`: The raw JSON of the request params (a valid JSON document,
///   or `"null"` if the request contained no params).
/// - `response`: The serialized response, see the return value.
///
/// Returns `true` if the command succeeded, with `response` containing the
/// raw JSON of the result value. Returns `false` if the command failed, with
/// `response` containing a JSON error object `{"code":..,"message":..}`.
pub type RpcDispatchFn = extern "C" fn(
  ctx: *mut c_void,
  method: &QString,
  params: &QString,
  response: &mut QString,
) -> bool;

/// Parse an error response of the C++ dispatch callback (see [RpcDispatchFn]).
fn parse_error_response(response: &str) -> Result<String, RpcError> {
  let value: Value = match serde_json::from_str(response) {
    Ok(value) => value,
    Err(_) => return Err(RpcError::internal_error()),
  };
  let code = value
    .get("code")
    .and_then(Value::as_i64)
    .unwrap_or(error_code::INTERNAL_ERROR);
  let message = value
    .get("message")
    .and_then(Value::as_str)
    .unwrap_or("Unknown error")
    .to_string();
  let mut error = RpcError::new(code, message);
  if let Some(data) = value.get("data") {
    error = error.with_data(data.clone());
  }
  Err(error)
}

/// Create a new [RpcServer] object.
///
/// - `token`: The authentication token (must not be empty).
/// - `protocol_version`: The protocol version, reported by `initialize`.
/// - `info`: A raw JSON object with application information (e.g. the
///   application version, the PID and the workspace path) which is merged
///   into the `initialize` response. If it is not a JSON object, it is
///   ignored.
/// - `dispatch`/`ctx`: The command dispatcher (see [RpcDispatchFn]).
#[no_mangle]
extern "C" fn ffi_rpc_server_new(
  token: &QString,
  protocol_version: &QString,
  info: &QString,
  dispatch: RpcDispatchFn,
  ctx: *mut c_void,
) -> *mut RpcServer {
  let callback = CppDispatch { f: dispatch, ctx };
  let dispatch: DispatchFn = Arc::new(move |method: &str, params: &str| {
    call_dispatch(callback, method, params)
  });
  let config = RpcServerConfig {
    token: from_qstring(token),
    protocol_version: from_qstring(protocol_version),
    info: serde_json::from_str(&from_qstring(info)).unwrap_or(Value::Null),
    dispatch,
  };
  Box::into_raw(Box::new(RpcServer::new(config)))
}

/// Set the OpenRPC API schema document served by the `rpc.discover` method.
#[no_mangle]
extern "C" fn ffi_rpc_server_set_api_schema(
  s: &mut RpcServer,
  schema: &QString,
) {
  s.set_api_schema(Some(from_qstring(schema)));
}

/// Start listening on 127.0.0.1 on the given port (0 = OS-assigned).
///
/// Returns `false` on failure, otherwise `*out_port` contains the actual
/// port the server is listening on.
#[no_mangle]
extern "C" fn ffi_rpc_server_start(
  s: &mut RpcServer,
  port: u16,
  out_port: &mut u16,
) -> bool {
  *out_port = 0;
  match s.start_on_port(port) {
    Ok(port) => {
      *out_port = port;
      true
    }
    Err(_) => false,
  }
}

/// Push an event (a JSON-RPC notification) to all connected clients.
///
/// This function never blocks, thus it is safe to be called from the GUI
/// thread. The `params` must be a valid JSON document (or empty, which is
/// treated as `null`).
#[no_mangle]
extern "C" fn ffi_rpc_server_notify(
  s: &mut RpcServer,
  method: &QString,
  params: &QString,
) {
  s.notify(&from_qstring(method), &from_qstring(params));
}

/// Delete [RpcServer] object (stops the server and joins its threads).
#[no_mangle]
extern "C" fn ffi_rpc_server_delete(s: *mut RpcServer) {
  assert!(!s.is_null());
  unsafe { drop(Box::from_raw(s)) };
}
