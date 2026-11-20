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

use rmpv::Value;

/// Message type of a request (client → server).
pub const MESSAGE_TYPE_REQUEST: i64 = 0;
/// Message type of a response (server → client).
pub const MESSAGE_TYPE_RESPONSE: i64 = 1;
/// Message type of a notification/event (server → all clients).
pub const MESSAGE_TYPE_NOTIFICATION: i64 = 2;
/// Message type of a request-scoped message (server → client, e.g.
/// progress or streamed data while a request is pending).
pub const MESSAGE_TYPE_REQUEST_MESSAGE: i64 = 3;
/// Message type of a cancel request (client → server).
pub const MESSAGE_TYPE_CANCEL: i64 = 4;

/// Name of the method which authenticates a connection. Must be the first
/// message of every connection, carrying the authentication token.
pub const METHOD_INITIALIZE: &str = "initialize";

/// Name of the protocol-level ping method (returns an empty map).
pub const METHOD_PING: &str = "ping";

/// RPC error codes.
///
/// The codes -32700..=-32600 follow the JSON-RPC 2.0 convention (reserved
/// protocol errors), the range -32000..=-32099 is reserved for
/// implementation defined server-errors (the application-specific codes).
pub mod error_code {
  /// The JSON sent is not a valid request (envelope doesn't conform).
  pub const INVALID_REQUEST: i64 = -32600;
  /// The method does not exist or is not available.
  pub const METHOD_NOT_FOUND: i64 = -32601;
  /// Invalid method parameter(s).
  pub const INVALID_PARAMS: i64 = -32602;
  /// Internal error.
  pub const INTERNAL_ERROR: i64 = -32603;
  /// The connection is not authenticated (or the token is wrong).
  pub const UNAUTHENTICATED: i64 = -32001;
  /// A command failed (application-specific error).
  pub const COMMAND_FAILED: i64 = -32002;
  /// The requested project is not open (application-specific error).
  pub const PROJECT_NOT_OPEN: i64 = -32003;
  /// The request was cancelled by the client.
  pub const CANCELLED: i64 = -32005;
}

/// An RPC error, as sent in a response.
#[derive(Clone, Debug)]
pub struct RpcError {
  /// Numeric error code (see [error_code]).
  code: i64,
  /// Short human-readable error description.
  message: String,
  /// Optional additional data, e.g. error details.
  data: Option<Value>,
}

impl RpcError {
  /// Create an error with a code and a message.
  pub fn new(code: i64, message: impl Into<String>) -> Self {
    Self {
      code,
      message: message.into(),
      data: None,
    }
  }

  /// Attach additional data to the error.
  pub fn with_data(mut self, data: Value) -> Self {
    self.data = Some(data);
    self
  }

  /// Create an error for a message which doesn't conform to the envelope
  /// (`-32600`).
  pub fn invalid_request() -> Self {
    Self::new(error_code::INVALID_REQUEST, "Invalid Request")
  }

  /// Create an error for an unknown method (`-32601`).
  pub fn method_not_found() -> Self {
    Self::new(error_code::METHOD_NOT_FOUND, "Method not found")
  }

  /// Create an error for invalid params (`-32602`).
  pub fn invalid_params() -> Self {
    Self::new(error_code::INVALID_PARAMS, "Invalid params")
  }

  /// Create an internal error (`-32603`).
  pub fn internal_error() -> Self {
    Self::new(error_code::INTERNAL_ERROR, "Internal error")
  }

  /// Get the error code.
  pub fn code(&self) -> i64 {
    self.code
  }

  /// Get the error message.
  pub fn message(&self) -> &str {
    &self.message
  }

  /// Serialize to a msgpack value (the `error` element of a response).
  pub fn to_value(&self) -> Value {
    let mut pairs = vec![
      (Value::from("code"), Value::from(self.code)),
      (Value::from("message"), Value::from(self.message.as_str())),
    ];
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
    /// The method name, e.g. "Project.list".
    method: String,
    /// The params (a map, possibly empty). Fully opaque to the server —
    /// the content is only interpreted by the command handlers.
    params: Value,
  },
  /// A cancel request: `[4, msgid]`.
  Cancel {
    /// The ID of the request to cancel.
    msgid: u32,
  },
  /// A decoded message which is not a valid client→server message: a
  /// known message type in the wrong direction (1, 2 or 3) or an unknown
  /// message type. Ignored (with a log message) for forward
  /// compatibility — the stream is still in sync.
  Ignored,
  /// A request which doesn't conform to the envelope (e.g. the method is
  /// not a string, the params are not a map or the msgid is not a valid
  /// u32). The stream is still in sync, so the connection stays open; if
  /// a msgid is extractable, it is echoed in an error response.
  Invalid {
    /// The request ID, if one could be extracted.
    msgid: Option<u32>,
  },
}

/// Classify an incoming message (which was already successfully decoded
/// as msgpack).
///
/// Strictness: The envelope must be an array whose first element is an
/// integer message type, the msgid must fit into u32, the method (for
/// requests) must be a string and the params must be a map (or nil/absent,
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
        Some(msgid) => msgid,
        None => return ParsedMessage::Invalid { msgid: None },
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
    // Known message types in the wrong direction, and unknown message
    // types: ignore for forward compatibility (the spec allows the
    // protocol to gain new message types with a higher protocolVersion).
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

/// Serialize a response: `[1, msgid, error, result]`.
///
/// Exactly one of `error`/`result` is sent: With an error, the result is
/// nil, and vice versa, as the protocol requires. The error is an
/// *arbitrary* msgpack value (as the protocol allows) — the Rust side
/// builds it with [RpcError::to_value], the FFI passes the error map
/// from C++ through unchanged (it may carry additional error properties
/// next to "code" and "message").
pub fn build_response(
  msgid: u32,
  error: Option<&Value>,
  result: &Value,
) -> Value {
  match error {
    Some(error) => Value::Array(vec![
      Value::from(MESSAGE_TYPE_RESPONSE),
      Value::from(msgid),
      error.clone(),
      Value::Nil,
    ]),
    None => Value::Array(vec![
      Value::from(MESSAGE_TYPE_RESPONSE),
      Value::from(msgid),
      Value::Nil,
      result.clone(),
    ]),
  }
}

/// Serialize a notification (an event broadcast to all clients):
/// `[2, method, params]`.
pub fn build_notification(method: &str, params: &Value) -> Value {
  Value::Array(vec![
    Value::from(MESSAGE_TYPE_NOTIFICATION),
    Value::from(method),
    params.clone(),
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

/// Extract the authentication token from the params of an `initialize`
/// request.
pub fn extract_token(params: &Value) -> Option<&str> {
  let pairs = match params {
    Value::Map(pairs) => pairs,
    _ => return None,
  };
  for (key, value) in pairs {
    if key.as_str() == Some("token") {
      return value.as_str();
    }
  }
  None
}

/// Build the `initialize` result: the configured application info (a map),
/// with the protocol version added/overridden.
pub fn build_initialize_result(info: &Value, protocol_version: &str) -> Value {
  let mut pairs = match info {
    Value::Map(pairs) => pairs.clone(),
    _ => Vec::new(), // Not a map: Ignore it (should never happen).
  };
  pairs.retain(|(key, _)| key.as_str() != Some("protocolVersion"));
  pairs.push((
    Value::from("protocolVersion"),
    Value::from(protocol_version),
  ));
  Value::Map(pairs)
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
      Value::from("Project.list"),
      Value::Map(vec![(Value::from("x"), Value::from(1))]),
    ]);
    match classify_message(&value) {
      ParsedMessage::Request {
        msgid,
        method,
        params,
      } => {
        assert_eq!(msgid, 42);
        assert_eq!(method, "Project.list");
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
      Value::from("Project.list"),
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
      Value::from("Project.list"),
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
    let value = Value::Array(vec![Value::from(4), Value::from(7)]);
    match classify_message(&value) {
      ParsedMessage::Cancel { msgid } => assert_eq!(msgid, 7),
      _ => panic!("expected a cancel"),
    }
  }

  #[test]
  fn test_classify_ignored() {
    // Wrong direction (client sent a response/notification/request msg):
    for msg_type in [1, 2, 3] {
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
    // Request with an invalid msgid (u64 > u32::MAX).
    let value = Value::Array(vec![
      Value::from(0),
      Value::from(u32::MAX as u64 + 1),
      Value::from("m"),
      Value::Nil,
    ]);
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Invalid { msgid: None }
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
  }

  #[test]
  fn test_error_to_value() {
    let error = RpcError::new(error_code::COMMAND_FAILED, "boom");
    let value = error.to_value();
    assert_eq!(field(&value, "code"), Some(&Value::from(-32002i64)));
    assert_eq!(field(&value, "message"), Some(&Value::from("boom")));
    assert_eq!(field(&value, "data"), None);
    let error = error.with_data(Value::from(7));
    let value = error.to_value();
    assert_eq!(field(&value, "data"), Some(&Value::from(7)));
  }

  #[test]
  fn test_build_response_success() {
    let response = build_response(5, None, &Value::from(true));
    let items = response.as_array().unwrap();
    assert_eq!(items[0], Value::from(1));
    assert_eq!(items[1], Value::from(5));
    assert_eq!(items[2], Value::Nil); // No error.
    assert_eq!(items[3], Value::from(true)); // The result.
  }

  #[test]
  fn test_build_response_error() {
    let error = RpcError::new(-32003, "nope");
    let response =
      build_response(5, Some(&error.to_value()), &Value::from(true));
    let items = response.as_array().unwrap();
    assert_eq!(items[0], Value::from(1));
    assert_eq!(items[1], Value::from(5));
    assert_eq!(field(&items[2], "code"), Some(&Value::from(-32003)));
    assert_eq!(items[3], Value::Nil); // Result is nil on error.
  }

  #[test]
  fn test_build_notification() {
    let notification = build_notification("project_opened", &Value::Nil);
    let items = notification.as_array().unwrap();
    assert_eq!(items[0], Value::from(2));
    assert_eq!(items[1], Value::from("project_opened"));
    assert_eq!(items[2], Value::Nil);
  }

  #[test]
  fn test_build_request_message() {
    let message = build_request_message(
      3,
      &Value::Map(vec![(Value::from("percent"), Value::from(50))]),
    );
    let items = message.as_array().unwrap();
    assert_eq!(items[0], Value::from(3));
    assert_eq!(items[1], Value::from(3));
    assert_eq!(field(&items[2], "percent"), Some(&Value::from(50)));
  }

  #[test]
  fn test_extract_token() {
    let params =
      Value::Map(vec![(Value::from("token"), Value::from("the-token"))]);
    assert_eq!(extract_token(&params), Some("the-token"));
    assert_eq!(extract_token(&Value::Map(Vec::new())), None);
    assert_eq!(extract_token(&Value::Nil), None);
  }

  #[test]
  fn test_build_initialize_result() {
    let info = Value::Map(vec![
      (Value::from("appVersion"), Value::from("1.2.3")),
      (Value::from("protocolVersion"), Value::from("old")), // Overridden.
    ]);
    let result = build_initialize_result(&info, "1");
    assert_eq!(field(&result, "appVersion"), Some(&Value::from("1.2.3")));
    assert_eq!(field(&result, "protocolVersion"), Some(&Value::from("1")));
    // Non-map info is ignored:
    let result = build_initialize_result(&Value::Nil, "1");
    assert_eq!(field(&result, "protocolVersion"), Some(&Value::from("1")));
    assert_eq!(result.as_map().unwrap().len(), 1);
  }
}
