//! Message envelope for the RPC server.
//!
//! The protocol is a custom, MessagePack-RPC-inspired design: Messages are
//! msgpack arrays whose first element is the message type (see
//! `dev/plans/rpc-server.md` for the full specification). This module
//! implements the *classification* of incoming messages and the
//! serialization of outgoing messages. It does deliberately not do any
//! framing (see [codec]) and it does not validate the message *content*
//! (e.g. the structure of `params`), which is up to the command handlers.
//!
//! The messages are built manually with [Value]s instead of using serde
//! derive, which gives full control over the exact structure without any
//! serialization error handling.

use super::rpc_api::RpcErrorCode;
use rmpv::Value;

/// The message type of a request (both directions).
const MESSAGE_TYPE_REQUEST: i64 = 0;
/// The message type of a response (both directions).
const MESSAGE_TYPE_RESPONSE: i64 = 1;
/// The message type of a request-scoped message (callee → caller, e.g.
/// progress or streamed data while a request is pending).
const MESSAGE_TYPE_REQUEST_MESSAGE: i64 = 2;
/// The message type of a cancel request (caller → callee).
const MESSAGE_TYPE_CANCEL: i64 = 3;

/// The msgid which is reserved for future notifications (see the protocol
/// documentation): It is not a valid request msgid, and it is used as the
/// echo msgid of error responses for requests whose msgid could not be
/// determined.
pub const RESERVED_MSGID: u32 = 0;

/// Name of the method which a connection must call first (handshake and
/// version negotiation). It is dispatched to the C++ side like any other
/// command, but — unlike all other methods except `authenticate` — it is
/// dispatched even while the connection is not authenticated yet.
pub const METHOD_INITIALIZE: &str = "initialize";

/// Name of the method which authenticates a connection. It is dispatched
/// to the C++ side like any other command, but — unlike all other methods
/// — its success reply authenticates the connection (see the reply path).
pub const METHOD_AUTHENTICATE: &str = "authenticate";

/// An RPC error, as sent in a response.
#[derive(Clone, Debug)]
pub struct RpcError {
  /// Numeric error code (see [ErrorCode]).
  code: RpcErrorCode,
  /// Short human-readable error description.
  message: String,
  /// Optional additional data, e.g. error details.
  data: Option<Value>,
}

impl RpcError {
  /// Create an error with a code and an optional application-specific
  /// detail message. The message sent on the wire is the canonical
  /// message of the error code (see the API specification), followed by
  /// the detail (if non-empty), separated by a colon.
  pub fn new(code: RpcErrorCode, detail: impl Into<String>) -> Self {
    Self {
      code,
      message: match detail.into() {
        detail if detail.is_empty() => code.message().to_string(),
        detail => format!("{}: {detail}", code.message()),
      },
      data: None,
    }
  }

  /// Create an error without an application-specific detail message, i.e.
  /// only the canonical message of the error code is sent.
  pub fn canonical(code: RpcErrorCode) -> Self {
    Self::new(code, "")
  }

  /// Attach additional data to the error.
  pub fn with_data(mut self, data: Value) -> Self {
    self.data = Some(data);
    self
  }

  /// Get the error code.
  pub fn code(&self) -> RpcErrorCode {
    self.code
  }

  /// Get the error message.
  pub fn message(&self) -> &str {
    &self.message
  }

  /// Serialize to the response payload of an error, i.e. a map with the
  /// message (plus optional data). The error code is *not* part of the
  /// payload — it is sent as the status of the response.
  pub fn to_payload(&self) -> Value {
    let mut pairs =
      vec![(Value::from("message"), Value::from(self.message.as_str()))];
    if let Some(data) = &self.data {
      pairs.push((Value::from("data"), data.clone()));
    }
    Value::Map(pairs)
  }
}

/// An incoming message, classified by its envelope.
#[derive(Clone, Debug)]
pub enum ParsedMessage {
  /// A request: `[0, msgid, method, params]`.
  Request {
    /// The request ID (unique per connection while pending), echoed back
    /// in the response.
    msgid: u32,
    /// The method name, e.g. "Project/list".
    method: String,
    /// The params (a map, possibly empty). Fully opaque to the server —
    /// the content is only interpreted by the command handlers.
    params: Value,
  },
  /// A cancel request: `[3, msgid]`.
  Cancel {
    /// The ID of the request to cancel.
    msgid: u32,
  },
  /// A decoded message which is not (yet) valid in this direction: a
  /// known message type which cannot be handled (1 or 2, until the
  /// protocol actually uses both directions) or an unknown message type.
  /// Ignored for forward compatibility — the stream is still in sync.
  Ignored,
  /// A request which doesn't conform to the envelope (e.g. the method is
  /// not a string, the params are not a map, the msgid is not a valid
  /// u32 or it is the reserved msgid 0). The stream is still in sync, so
  /// the connection stays open; the request is answered with an error
  /// response, echoed to the msgid if one could be extracted (otherwise
  /// to the reserved msgid 0).
  Invalid {
    /// The request ID, if one could be extracted.
    msgid: Option<u32>,
  },
}

/// Classify an incoming message (which was already successfully decoded
/// as msgpack).
///
/// Strictness: The envelope must be an array whose first element is an
/// integer message type, the msgid must fit into u32 (and must not be the
/// reserved msgid 0), the method (for requests) must be a string and the
/// params must be a map (or nil/absent,
/// both of which are normalized to an empty map). The *content* of the
/// params is not validated at all, since that is up to the command
/// handlers.
pub fn classify_message(value: &Value) -> ParsedMessage {
  let items = match value {
    Value::Array(items) if !items.is_empty() => items,
    _ => return ParsedMessage::Invalid { msgid: None },
  };
  let msg_type = match items.first().and_then(Value::as_i64) {
    Some(msg_type) => msg_type,
    None => return ParsedMessage::Invalid { msgid: None },
  };
  // Best-effort extraction of the msgid (at index 1 for all types which
  // have one), to be echoed in error responses.
  let extract_msgid = || -> Option<u32> {
    let msgid = items.get(1)?.as_u64()?;
    u32::try_from(msgid).ok()
  };
  match msg_type {
    MESSAGE_TYPE_REQUEST => {
      let msgid = match extract_msgid() {
        Some(msgid) if msgid != RESERVED_MSGID => msgid,
        // The reserved msgid 0 is echoed in the error response (it also
        // covers the case where no msgid could be extracted).
        _ => return ParsedMessage::Invalid { msgid: Some(0) },
      };
      let method = match items.get(2).and_then(Value::as_str) {
        Some(method) => method.to_string(),
        None => return ParsedMessage::Invalid { msgid: Some(msgid) },
      };
      let params = normalize_params(items.get(3));
      match params {
        Some(params) => ParsedMessage::Request {
          msgid,
          method,
          params,
        },
        None => ParsedMessage::Invalid { msgid: Some(msgid) },
      }
    }
    MESSAGE_TYPE_CANCEL => match extract_msgid() {
      Some(msgid) => ParsedMessage::Cancel { msgid },
      None => ParsedMessage::Invalid { msgid: None },
    },
    // Known message types which cannot be handled in this direction (1
    // and 2, until the protocol actually uses both directions), and
    // unknown message types: ignore for forward compatibility — the
    // stream is still in sync.
    _ => ParsedMessage::Ignored,
  }
}

/// Normalize the params of a request: maps are passed through, nil (or a
/// missing element) becomes an empty map. Any other type is invalid.
fn normalize_params(value: Option<&Value>) -> Option<Value> {
  match value {
    None | Some(Value::Nil) => Some(Value::Map(Vec::new())),
    Some(value @ Value::Map(_)) => Some(value.clone()),
    Some(_) => None,
  }
}

/// Serialize a response: `[1, msgid, status, payload]`.
///
/// The status is `0` on success (the payload carries the result, which
/// may be nil), or the error code on failure (the payload is a map with
/// the error message, see [RpcError::to_payload]).
pub fn build_response(msgid: u32, status: i64, payload: &Value) -> Value {
  Value::Array(vec![
    Value::from(MESSAGE_TYPE_RESPONSE),
    Value::from(msgid),
    Value::from(status),
    payload.clone(),
  ])
}

/// Serialize a request-scoped message (e.g. progress, or streamed partial
/// data): `[3, msgid, payload]`.
pub fn build_request_message(msgid: u32, payload: &Value) -> Value {
  Value::Array(vec![
    Value::from(MESSAGE_TYPE_REQUEST_MESSAGE),
    Value::from(msgid),
    payload.clone(),
  ])
}

#[cfg(test)]
mod tests {
  use super::*;

  /// Get a field of a map value (msgpack maps are ordered lists of pairs,
  /// not lookup structures, so we search linearly — like a decoder which
  /// doesn't know the key order).
  fn field<'a>(value: &'a Value, key: &str) -> Option<&'a Value> {
    let pairs = value.as_map()?;
    pairs
      .iter()
      .find(|(k, _)| k.as_str() == Some(key))
      .map(|(_, v)| v)
  }

  #[test]
  fn test_classify_request() {
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(42u32),
      Value::from("Project/list"),
      Value::Map(vec![(Value::from("x"), Value::from(1))]),
    ]);
    match classify_message(&value) {
      ParsedMessage::Request {
        msgid,
        method,
        params,
      } => {
        assert_eq!(msgid, 42);
        assert_eq!(method, "Project/list");
        assert_eq!(field(&params, "x"), Some(&Value::from(1)));
      }
      _ => panic!("expected a request"),
    }
  }

  #[test]
  fn test_classify_request_with_nil_params() {
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(1),
      Value::from("Project/list"),
      Value::Nil,
    ]);
    match classify_message(&value) {
      ParsedMessage::Request { params, .. } => {
        assert_eq!(params, Value::Map(Vec::new())); // Normalized to empty map.
      }
      _ => panic!("expected a request"),
    }
  }

  #[test]
  fn test_classify_request_without_params() {
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(1),
      Value::from("Project/list"),
    ]);
    match classify_message(&value) {
      ParsedMessage::Request { params, .. } => {
        assert_eq!(params, Value::Map(Vec::new())); // Normalized to empty map.
      }
      _ => panic!("expected a request"),
    }
  }

  #[test]
  fn test_classify_cancel() {
    let value = Value::Array(vec![Value::from(3), Value::from(7)]);
    match classify_message(&value) {
      ParsedMessage::Cancel { msgid } => assert_eq!(msgid, 7),
      _ => panic!("expected a cancel"),
    }
  }

  #[test]
  fn test_classify_ignored() {
    // Types which cannot be handled in this direction (client sent a
    // response / request-scoped message; until the protocol actually
    // uses both directions):
    for msg_type in [1, 2] {
      let value = Value::Array(vec![Value::from(msg_type), Value::from(1)]);
      assert!(matches!(classify_message(&value), ParsedMessage::Ignored));
    }
    // Unknown message types are ignored, too (forward compatibility).
    let value = Value::Array(vec![Value::from(99), Value::from(1)]);
    assert!(matches!(classify_message(&value), ParsedMessage::Ignored));
  }

  #[test]
  fn test_classify_invalid() {
    // Not an array.
    assert!(matches!(
      classify_message(&Value::from(42)),
      ParsedMessage::Invalid { msgid: None }
    ));
    // Empty array.
    assert!(matches!(
      classify_message(&Value::Array(Vec::new())),
      ParsedMessage::Invalid { msgid: None }
    ));
    // Message type not an integer.
    assert!(matches!(
      classify_message(&Value::Array(vec![Value::from("0")])),
      ParsedMessage::Invalid { msgid: None }
    ));
    // Request with an invalid msgid (u64 > u32::MAX): No valid msgid
    // could be extracted, so the error response is echoed to msgid 0.
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(u32::MAX as u64 + 1),
      Value::from("m"),
      Value::Nil,
    ]);
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Invalid { msgid: Some(0) }
    ));
    // Request with a non-string method, but the msgid is echoed.
    let value =
      Value::Array(vec![Value::from(0), Value::from(9), Value::from(42)]);
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Invalid { msgid: Some(9) }
    ));
    // Request with non-map params.
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(9),
      Value::from("m"),
      Value::from("params"),
    ]);
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Invalid { msgid: Some(9) }
    ));
    // The reserved msgid 0 is rejected, with the error echoed to msgid 0.
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(0),
      Value::from("m"),
      Value::Nil,
    ]);
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Invalid { msgid: Some(0) }
    ));
  }

  #[test]
  fn test_error_to_payload() {
    let error = RpcError::new(RpcErrorCode::CommandFailed, "boom");
    let value = error.to_payload();
    assert_eq!(field(&value, "code"), None); // Sent as the status instead.
    assert_eq!(
      field(&value, "message"),
      Some(&Value::from("Command failed: boom"))
    );
    assert_eq!(field(&value, "data"), None);
    let error = error.with_data(Value::from(7));
    let value = error.to_payload();
    assert_eq!(field(&value, "data"), Some(&Value::from(7)));
  }

  #[test]
  fn test_build_response_success() {
    let response = build_response(5, 0, &Value::from(true));
    let items = response.as_array().unwrap();
    assert_eq!(items[0], Value::from(1));
    assert_eq!(items[1], Value::from(5));
    assert_eq!(items[2], Value::from(0)); // Status: success.
    assert_eq!(items[3], Value::from(true)); // The payload.
  }

  #[test]
  fn test_build_response_error() {
    let error = RpcError::new(RpcErrorCode::InvalidHandle, "nope");
    let response = build_response(5, error.code() as i64, &error.to_payload());
    let items = response.as_array().unwrap();
    assert_eq!(items[0], Value::from(1));
    assert_eq!(items[1], Value::from(5));
    assert_eq!(items[2], Value::from(RpcErrorCode::InvalidHandle as i64)); // Status: the error code.
    assert_eq!(
      field(&items[3], "message"),
      Some(&Value::from("Invalid object handle: nope"))
    ); // The payload.
  }

  #[test]
  fn test_build_request_message() {
    let message = build_request_message(
      3,
      &Value::Map(vec![(Value::from("percent"), Value::from(50))]),
    );
    let items = message.as_array().unwrap();
    assert_eq!(items[0], Value::from(2));
    assert_eq!(items[1], Value::from(3));
    assert_eq!(field(&items[2], "percent"), Some(&Value::from(50)));
  }
}
