//! FFI for [rpc_server], to be used from C++ through cbindgen.
//!
//! This module is the only place where the msgpack world meets the C++
//! world: Command params cross the FFI as `QVariant` trees (built from
//! msgpack values), and responses/notifications cross back as `QVariant`
//! trees (read back into msgpack values). The conversion only accepts the
//! protocol's restricted value domain (nil, bool, i64, f64, string,
//! bytearray, list, map with string keys) — values outside that domain
//! (msgpack `ext`, u64 > i64::MAX, non-string map keys, invalid UTF-8)
//! are rejected as payload-level errors.

use super::cpp_ffi::*;
use crate::rpc_server::{
  CancelFn, DispatchFn, DispatchOutcome, RpcError, RpcErrorCode, RpcServer,
};
use std::ffi::c_void;
use std::sync::Arc;

use rmpv::Value;

/// Callback which dispatches a command request from the Rust server to C++.
///
/// It is invoked on the connection's reader thread with
/// `(ctx, conn_id, msg_id, method, params)`, where `params` is the whole
/// params map (including the reserved `obj` key, which is a documented
/// convention and extracted by the C++ dispatch layer, not by Rust).
///
/// The call is fire-and-forget: The C++ implementation marshals the request
/// to the GUI thread (it must never block the calling thread!) and must
/// eventually respond exactly once via `ffi_rpc_server_reply()`, from any
/// thread. It may send request-scoped messages (e.g. progress) via
/// `ffi_rpc_server_notify_conn()` in the meantime.
///
/// Returns `true` if the request was accepted (i.e. a response will be
/// sent), or `false` if it could not be dispatched (the server then
/// responds with an internal error).
pub type RpcOnRequestCallback = extern "C" fn(
  ctx: *mut c_void,
  conn_id: u32,
  msg_id: u32,
  method: &QString,
  params: &QVariant,
) -> bool;

/// Callback which is invoked when a client cancels a pending request
/// (message `[4, msg_id]`).
///
/// It is invoked on the connection's reader thread. The C++ implementation
/// must eventually make sure the request is replied exactly once
/// (typically with a "cancelled" error, or with the normal result if the
/// work already completed).
pub type RpcOnCancelCallback =
  extern "C" fn(ctx: *mut c_void, conn_id: u32, msg_id: u32);

/// A C++ callback (function pointer + context).
///
/// <div class="warning">
/// A raw function pointer is not [Send]/[Sync] if its signature contains
/// raw pointers, so we wrap both the callback and its context in a newtype.
/// The C++ side guarantees that they stay valid for the whole lifetime of
/// the server, thus it is safe to share them between the connection
/// threads.
/// </div>
struct CppCallback<F> {
  /// The callback function.
  f: F,
  /// The context pointer passed to the callback.
  ctx: *mut c_void,
}

// SAFETY: See the warning on [CppCallback].
unsafe impl<F> Send for CppCallback<F> {}
// SAFETY: See the warning on [CppCallback].
unsafe impl<F> Sync for CppCallback<F> {}

/// Call the C++ dispatch callback with the given request.
///
/// Returns `Ok(true)` if the request was accepted, `Ok(false)` if the C++
/// side rejected it, or `Err` if the params are outside the protocol's
/// value domain (payload-level error, answered with "invalid params").
///
/// This is a separate function (instead of inlining it into the closure
/// below) on purpose: Rust's disjoint closure captures would otherwise
/// capture the individual (raw) fields of [CppCallback], which would make
/// the closure not [Send]/[Sync] although the struct itself is.
fn call_dispatch(
  callback: &CppCallback<RpcOnRequestCallback>,
  conn_id: u32,
  msg_id: u32,
  method: &str,
  params: &Value,
) -> Result<bool, ()> {
  let method_q = qstring_new();
  assert!(!method_q.is_null());
  unsafe { qstring_set(&mut *method_q, method) };
  let params_q = match qvariant_from_msgpack(params) {
    Ok(obj) => obj,
    Err(()) => {
      qstring_delete(method_q);
      return Err(());
    }
  };
  let ok = unsafe {
    (callback.f)(callback.ctx, conn_id, msg_id, &*method_q, &*params_q)
  };
  qstring_delete(method_q);
  qvariant_delete(params_q);
  Ok(ok)
}

/// Call the C++ cancel callback (same pattern as [call_dispatch]).
fn call_cancel(
  callback: &CppCallback<RpcOnCancelCallback>,
  conn_id: u32,
  msg_id: u32,
) {
  (callback.f)(callback.ctx, conn_id, msg_id);
}

/// Build a QVariant tree from a msgpack value (recursively). The returned
/// object must be freed with [qvariant_delete].
///
/// Values outside the protocol's value domain (see the module
/// documentation) are rejected.
fn qvariant_from_msgpack(value: &Value) -> Result<*mut QVariant, ()> {
  let obj = qvariant_new();
  assert!(!obj.is_null());
  if build_variant(obj, value).is_err() {
    qvariant_delete(obj);
    return Err(());
  }
  Ok(obj)
}

/// Recursively fill a QVariant with a msgpack value.
fn build_variant(obj: *mut QVariant, value: &Value) -> Result<(), ()> {
  unsafe {
    match value {
      Value::Nil => ffi_qvariant_set_nil(&mut *obj),
      Value::Boolean(b) => ffi_qvariant_set_bool(&mut *obj, *b),
      Value::Integer(i) => {
        let i = i.as_i64().ok_or(())?; // u64 > i64::MAX is rejected.
        ffi_qvariant_set_i64(&mut *obj, i);
      }
      Value::F32(f) => ffi_qvariant_set_f64(&mut *obj, *f as f64), // Widened exactly.
      Value::F64(f) => ffi_qvariant_set_f64(&mut *obj, *f),
      Value::String(s) => {
        let s = s.as_str().ok_or(())?; // Invalid UTF-8 is rejected.
        ffi_qvariant_set_str(&mut *obj, s.as_ptr().cast(), s.len());
      }
      Value::Binary(b) => {
        ffi_qvariant_set_bytearray(&mut *obj, b.as_ptr(), b.len())
      }
      Value::Array(items) => {
        ffi_qvariant_set_empty_list(&mut *obj);
        for item in items {
          let child = qvariant_from_msgpack(item)?;
          ffi_qvariant_list_append(&mut *obj, &*child);
          qvariant_delete(child);
        }
      }
      Value::Map(pairs) => {
        ffi_qvariant_set_empty_map(&mut *obj);
        for (key, item) in pairs {
          let key = key.as_str().ok_or(())?; // Non-string keys are rejected.
          let child = qvariant_from_msgpack(item)?;
          ffi_qvariant_map_insert(
            &mut *obj,
            key.as_ptr().cast(),
            key.len(),
            &*child,
          );
          qvariant_delete(child);
        }
      }
      Value::Ext(..) => return Err(()), // Not part of the protocol.
    }
  }
  Ok(())
}

/// Read a QVariant tree into a msgpack value (recursively). Fails if the
/// QVariant contains a metatype outside the protocol's value domain (e.g.
/// a type the C++ side forgot to normalize).
fn qvariant_to_msgpack(obj: &QVariant) -> Result<Value, ()> {
  unsafe {
    Ok(match ffi_qvariant_type(obj) {
      qmetatype::UNKNOWN => Value::Nil,
      qmetatype::BOOL => Value::from(ffi_qvariant_get_bool(obj)),
      qmetatype::LONGLONG => Value::from(ffi_qvariant_get_i64(obj)),
      qmetatype::DOUBLE => Value::from(ffi_qvariant_get_f64(obj)),
      qmetatype::QSTRING => {
        let out = qstring_new();
        assert!(!out.is_null());
        ffi_qvariant_get_str(obj, &mut *out);
        let s = from_qstring(&*out);
        qstring_delete(out);
        Value::from(s)
      }
      qmetatype::QBYTEARRAY => {
        let len = ffi_qvariant_get_bytearray_len(obj);
        let mut buf = vec![0u8; len];
        ffi_qvariant_get_bytearray_data(obj, buf.as_mut_ptr());
        Value::Binary(buf)
      }
      qmetatype::QVARIANTLIST => {
        let len = ffi_qvariant_list_len(obj);
        let mut items = Vec::with_capacity(len);
        for index in 0..len {
          let child = qvariant_new();
          assert!(!child.is_null());
          ffi_qvariant_list_at(obj, index, &mut *child);
          let value = qvariant_to_msgpack(&*child);
          qvariant_delete(child);
          items.push(value?);
        }
        Value::Array(items)
      }
      qmetatype::QVARIANTMAP => {
        let len = ffi_qvariant_map_len(obj);
        let mut pairs = Vec::with_capacity(len);
        for index in 0..len {
          let key_q = qstring_new();
          assert!(!key_q.is_null());
          let child = qvariant_new();
          assert!(!child.is_null());
          ffi_qvariant_map_at(obj, index, &mut *key_q, &mut *child);
          let key = from_qstring(&*key_q);
          qstring_delete(key_q);
          let value = qvariant_to_msgpack(&*child);
          qvariant_delete(child);
          pairs.push((Value::from(key), value?));
        }
        Value::Map(pairs)
      }
      // Any other metatype is not part of the protocol's value domain.
      _ => return Err(()),
    })
  }
}

/// Look up a field of a msgpack map value.
fn map_field<'a>(value: &'a Value, key: &str) -> Option<&'a Value> {
  value
    .as_map()?
    .iter()
    .find(|(k, _)| k.as_str() == Some(key))
    .map(|(_, v)| v)
}

/// Check whether a msgpack value is a valid error object, as produced by
/// the C++ dispatch layer: `{"code": <int>, "message": <string>,
/// "data"?: <any>, ...}`. Additional fields next to "code" and "message"
/// (error properties, see the API specification) are allowed — the whole
/// error map is passed to the client unchanged.
fn is_error_object(value: &Value) -> bool {
  map_field(value, "code").and_then(Value::as_i64).is_some()
    && map_field(value, "message")
      .and_then(Value::as_str)
      .is_some()
}

/// Create a new [RpcServer] object, which immediately starts listening on
/// 127.0.0.1 (see `DEFAULT_PORT` / `MAX_PORT_ATTEMPTS` in
/// `rpc_server.rs` for the port selection).
///
/// - `dispatch`/`cancel`/`ctx`: The command dispatcher and cancel callback.
///   The `initialize` request (authentication) is dispatched through the
///   dispatcher as well — the C++ side validates the token and calls
///   `ffi_rpc_server_set_authenticated()`.
///
/// Returns null if the server could not be started (no free port / thread
/// spawning failed).
#[no_mangle]
extern "C" fn ffi_rpc_server_new(
  on_request: RpcOnRequestCallback,
  on_cancel: RpcOnCancelCallback,
  ctx: *mut c_void,
  port: &mut u16,
) -> *mut RpcServer {
  let dispatch_callback = CppCallback { f: on_request, ctx };
  let dispatch: DispatchFn = Arc::new(
    move |conn_id: u32, msg_id: u32, method: &str, params: &Value| {
      match call_dispatch(&dispatch_callback, conn_id, msg_id, method, params) {
        Ok(true) => DispatchOutcome::Dispatched,
        Ok(false) => DispatchOutcome::ReplyNow(RpcError::new(
          RpcErrorCode::InternalError,
          "Failed to dispatch the command",
        )),
        Err(()) => {
          // Values outside the protocol's value domain are payload-level
          // errors: The message was decoded fine, so the connection stays
          // open and the request is answered with "invalid params".
          eprintln!(
            "librepcb-rust: Rejecting params of '{method}': outside the value domain"
          );
          DispatchOutcome::ReplyNow(RpcError::invalid_params())
        }
      }
    },
  );
  let cancel_callback = CppCallback { f: on_cancel, ctx };
  let cancel: CancelFn = Arc::new(move |conn_id, msg_id| {
    call_cancel(&cancel_callback, conn_id, msg_id)
  });
  match RpcServer::new(dispatch, cancel, port) {
    Ok(server) => Box::into_raw(Box::new(server)),
    Err(e) => {
      eprintln!("librepcb-rust: Failed to start the RPC server: {e}");
      std::ptr::null_mut()
    }
  }
}

/// Delete [RpcServer] object (stops the server and joins its threads).
#[no_mangle]
extern "C" fn ffi_rpc_server_delete(s: *mut RpcServer) {
  assert!(!s.is_null());
  unsafe { drop(Box::from_raw(s)) };
}

/// Mark a connection as (un)authenticated.
///
/// Called by the C++ side after it validated the authentication token of
/// an `initialize` request (the token handling lives on the C++ side).
/// Calls for unknown or already closed connections are ignored.
#[no_mangle]
extern "C" fn ffi_rpc_server_set_authenticated(
  s: &mut RpcServer,
  conn_id: u32,
  authenticated: bool,
) {
  s.set_authenticated(conn_id, authenticated)
}

/// Send a request-scoped message (e.g. a progress report) to the client
/// which sent the request (never blocks).
///
/// The message is only delivered while the request is pending, i.e. before
/// its response was sent (returns `false` otherwise). If `payload`
/// contains types outside the protocol's value domain, it is dropped with
/// a warning.
#[no_mangle]
extern "C" fn ffi_rpc_server_notify(
  s: &mut RpcServer,
  conn_id: u32,
  msg_id: u32,
  payload: &QVariant,
) -> bool {
  let Ok(payload) = qvariant_to_msgpack(payload) else {
    eprintln!("librepcb-rust: Ignoring request message with invalid payload.");
    return false;
  };
  s.notify_conn(conn_id, msg_id, &payload)
}

/// Send the response for a pending request (thread-safe, never blocks).
///
/// Exactly one of result/error is sent, enforced by the server: A second
/// response for the same msg_id, or a response for an unknown or already
/// closed connection, is silently dropped (returns `false`).
///
/// - `ok == true`: `value` is the result.
/// - `ok == false`: `value` is an error map `{"code": <int>,
///   "message": <string>, "data"?: <any>}`.
///
/// If `value` contains types outside the protocol's value domain, an
/// internal error is sent instead.
#[no_mangle]
extern "C" fn ffi_rpc_server_reply(
  s: &mut RpcServer,
  conn_id: u32,
  msg_id: u32,
  ok: bool,
  payload: &QVariant,
) -> bool {
  let payload = match qvariant_to_msgpack(payload) {
    Ok(payload) => payload,
    Err(()) => {
      eprintln!("librepcb-rust: A command returned an unsupported value type.");
      return s.reply(
        conn_id,
        msg_id,
        Some(&RpcError::new(
          RpcErrorCode::InternalError,
          "The command returned an unsupported value type",
        )),
        &Value::Nil,
      );
    }
  };
  if ok {
    s.reply(conn_id, msg_id, None, &payload)
  } else {
    // Pass the error map from C++ through unchanged — it may carry
    // additional error properties next to "code" and "message" (see the
    // error types in the API specification). Only if it is not even a
    // valid error object, fall back to a generic internal error.
    if !is_error_object(&payload) {
      eprintln!("librepcb-rust: A command returned an invalid error object.");
      return s.reply(
        conn_id,
        msg_id,
        Some(&RpcError::new(
          RpcErrorCode::InternalError,
          "The command returned an invalid error object",
        )),
        &Value::Nil,
      );
    }
    s.reply_error(conn_id, msg_id, &payload)
  }
}
