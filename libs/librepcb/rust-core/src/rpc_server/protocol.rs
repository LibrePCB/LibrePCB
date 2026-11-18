//! JSON-RPC 2.0 protocol types for the TCP RPC server.
//!
//! This module implements the message *envelope*, i.e. the classification
//! of incoming messages and the serialization of outgoing messages. It does
//! deliberately not do any framing (see [codec]) or transport handling (see
//! [connection]), and it does not validate the message *content* (e.g. the
//! structure of `params`), which is up to the command handlers.
//!
//! The messages are built manually with `serde_json` values instead of using
//! serde derive, which gives full control over the exact key layout without
//! any serialization error handling (a `serde_json::Value` can always be
//! serialized).

use serde_json::{Map, Value};

/// Name of the method which authenticates a connection. Must be the first
/// message of every connection, carrying the authentication token.
pub const METHOD_INITIALIZE: &str = "initialize";

/// Name of the protocol-level ping method (returns an empty result).
pub const METHOD_PING: &str = "ping";

/// Name of the OpenRPC discovery method which returns the API schema.
pub const METHOD_RPC_DISCOVER: &str = "rpc.discover";

/// cbindgen:ignore
///
/// JSON-RPC 2.0 error codes.
///
/// The codes -32700..=-32600 are defined by the JSON-RPC 2.0 specification
/// as reserved "protocol errors". The range -32000..=-32099 is reserved for
/// "implementation defined server-errors", which is where the
/// application-specific codes live (they are used by the command handlers,
/// i.e. by the C++ side).
pub mod error_code {
  /// Invalid JSON was received by the server.
  pub const PARSE_ERROR: i64 = -32700;
  /// The JSON sent is not a valid JSON-RPC 2.0 request.
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
  /// The server is shutting down.
  pub const SERVER_SHUTTING_DOWN: i64 = -32004;
}

/// A JSON-RPC 2.0 error object, as sent in a response.
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

  /// Create an error for invalid JSON (`-32700`).
  pub fn parse_error() -> Self {
    Self::new(error_code::PARSE_ERROR, "Parse error")
  }

  /// Create an error for a message which is not a valid JSON-RPC 2.0
  /// request (`-32600`).
  pub fn invalid_request() -> Self {
    Self::new(error_code::INVALID_REQUEST, "Invalid Request")
  }

  /// Create an error for an unknown method (`-32601`).
  pub fn method_not_found() -> Self {
    Self::new(error_code::METHOD_NOT_FOUND, "Method not found")
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

  /// Serialize to a JSON value (the `error` object of a response).
  pub fn to_value(&self) -> Value {
    let mut obj = Map::new();
    obj.insert("code".to_string(), Value::from(self.code));
    obj.insert("message".to_string(), Value::from(self.message.as_str()));
    if let Some(data) = &self.data {
      obj.insert("data".to_string(), data.clone());
    }
    Value::Object(obj)
  }
}

/// An incoming message, classified by its JSON-RPC 2.0 envelope.
#[derive(Clone, Debug)]
pub enum ParsedMessage {
  /// A request (contains an `id` and thus requires a response).
  Request {
    /// The request ID, to be echoed back in the response.
    id: Value,
    /// The method name.
    method: String,
    /// The optional parameters, as raw JSON of arbitrary structure.
    params: Option<Value>,
  },
  /// A notification (contains no `id`, so the server must not respond).
  ///
  /// The method and params are not carried since the protocol does not
  /// define any client->server notifications — the server ignores them.
  Notification,
  /// A response (must never be sent by a client, but is tolerated).
  Response,
  /// Valid JSON, but not a valid JSON-RPC 2.0 request or notification.
  ///
  /// The `id` is extracted on a best-effort basis so the error response can
  /// echo it back.
  Invalid {
    /// The request ID, if one was present.
    id: Option<Value>,
  },
}

/// Classify an incoming message (which was already successfully parsed as
/// JSON).
///
/// Strictness: The `jsonrpc` field must be exactly `"2.0"` and `method` (if
/// present) must be a string, as required by the JSON-RPC 2.0 specification.
/// The `params` field is not validated at all, since its structure is up to
/// the command handlers.
pub fn classify_message(value: &Value) -> ParsedMessage {
  let obj = match value.as_object() {
    Some(obj) => obj,
    None => return ParsedMessage::Invalid { id: None },
  };
  let id = obj.get("id").cloned();
  match obj.get("jsonrpc") {
    Some(Value::String(version)) if version == "2.0" => {}
    _ => return ParsedMessage::Invalid { id },
  }
  let method = match obj.get("method") {
    Some(Value::String(method)) => method.clone(),
    Some(_) => return ParsedMessage::Invalid { id },
    None => {
      return if obj.contains_key("result") || obj.contains_key("error") {
        ParsedMessage::Response
      } else {
        ParsedMessage::Invalid { id }
      };
    }
  };
  match id {
    Some(id) => ParsedMessage::Request {
      id,
      method,
      params: obj.get("params").cloned(),
    },
    None => ParsedMessage::Notification,
  }
}

/// Serialize a JSON-RPC 2.0 response containing a result.
pub fn build_result_response(id: Value, result: Value) -> String {
  let mut obj = Map::new();
  obj.insert("jsonrpc".to_string(), Value::from("2.0"));
  obj.insert("id".to_string(), id);
  obj.insert("result".to_string(), result);
  Value::Object(obj).to_string()
}

/// Serialize a JSON-RPC 2.0 response containing an error.
pub fn build_error_response(id: Value, error: &RpcError) -> String {
  let mut obj = Map::new();
  obj.insert("jsonrpc".to_string(), Value::from("2.0"));
  obj.insert("id".to_string(), id);
  obj.insert("error".to_string(), error.to_value());
  Value::Object(obj).to_string()
}

/// Serialize a JSON-RPC 2.0 notification (contains no `id`, so no response
/// is expected from the receiver).
pub fn build_notification(method: &str, params: &Value) -> String {
  let mut obj = Map::new();
  obj.insert("jsonrpc".to_string(), Value::from("2.0"));
  obj.insert("method".to_string(), Value::from(method));
  obj.insert("params".to_string(), params.clone());
  Value::Object(obj).to_string()
}

#[cfg(test)]
mod tests {
  use super::*;
  use serde_json::json;

  #[test]
  fn test_classify_request() {
    let value = json!({"jsonrpc": "2.0", "id": 42, "method": "project/list",
                       "params": {"a": 1}});
    match classify_message(&value) {
      ParsedMessage::Request { id, method, params } => {
        assert_eq!(id, json!(42));
        assert_eq!(method, "project/list");
        assert_eq!(params, Some(json!({"a": 1})));
      }
      _ => panic!("expected a request"),
    }
  }

  #[test]
  fn test_classify_request_without_params() {
    let value = json!({"jsonrpc": "2.0", "id": "abc", "method": "ping"});
    match classify_message(&value) {
      ParsedMessage::Request { id, method, params } => {
        assert_eq!(id, json!("abc"));
        assert_eq!(method, "ping");
        assert_eq!(params, None);
      }
      _ => panic!("expected a request"),
    }
  }

  #[test]
  fn test_classify_notification() {
    let value = json!({"jsonrpc": "2.0", "method": "event/changed"});
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Notification
    ));
    // Also with params:
    let value =
      json!({"jsonrpc": "2.0", "method": "event/changed", "params": 7});
    assert!(matches!(
      classify_message(&value),
      ParsedMessage::Notification
    ));
  }

  #[test]
  fn test_classify_response() {
    assert!(matches!(
      classify_message(&json!({"jsonrpc": "2.0", "result": 42, "id": 1})),
      ParsedMessage::Response
    ));
    assert!(matches!(
      classify_message(
        &json!({"jsonrpc": "2.0", "error": {"code": 1}, "id": 1})
      ),
      ParsedMessage::Response
    ));
  }

  #[test]
  fn test_classify_invalid() {
    // Not an object.
    assert!(matches!(
      classify_message(&json!(42)),
      ParsedMessage::Invalid { id: None }
    ));
    // Missing or wrong protocol version.
    assert!(matches!(
      classify_message(&json!({"id": 1, "method": "ping"})),
      ParsedMessage::Invalid { id: Some(_) }
    ));
    assert!(matches!(
      classify_message(&json!({"jsonrpc": "1.0", "id": 1, "method": "ping"})),
      ParsedMessage::Invalid { id: Some(_) }
    ));
    // Method not a string.
    assert!(matches!(
      classify_message(&json!({"jsonrpc": "2.0", "id": 1, "method": 42})),
      ParsedMessage::Invalid { id: Some(_) }
    ));
    // Neither request, notification nor response.
    assert!(matches!(
      classify_message(&json!({"jsonrpc": "2.0"})),
      ParsedMessage::Invalid { id: None }
    ));
  }

  #[test]
  fn test_error_to_value() {
    let error = RpcError::new(error_code::COMMAND_FAILED, "boom");
    assert_eq!(error.to_value(), json!({"code": -32002, "message": "boom"}));
    let error = error.with_data(json!({"details": 7}));
    assert_eq!(
      error.to_value(),
      json!({"code": -32002, "message": "boom", "data": {"details": 7}})
    );
  }

  #[test]
  fn test_build_result_response() {
    let response: Value =
      serde_json::from_str(&build_result_response(json!(5), json!({"x": 1})))
        .unwrap();
    assert_eq!(response.get("jsonrpc"), Some(&json!("2.0")));
    assert_eq!(response.get("id"), Some(&json!(5)));
    assert_eq!(response.get("result"), Some(&json!({"x": 1})));
    assert!(response.get("error").is_none());
  }

  #[test]
  fn test_build_error_response() {
    let response: Value = serde_json::from_str(&build_error_response(
      Value::Null,
      &RpcError::parse_error(),
    ))
    .unwrap();
    assert_eq!(response.get("jsonrpc"), Some(&json!("2.0")));
    assert_eq!(response.get("id"), Some(&Value::Null));
    assert_eq!(
      response.get("error"),
      Some(&json!({"code": -32700, "message": "Parse error"}))
    );
    assert!(response.get("result").is_none());
  }

  #[test]
  fn test_build_notification() {
    let notification: Value =
      serde_json::from_str(&build_notification("event/x", &json!({"a": 1})))
        .unwrap();
    assert_eq!(notification.get("jsonrpc"), Some(&json!("2.0")));
    assert_eq!(notification.get("method"), Some(&json!("event/x")));
    assert_eq!(notification.get("params"), Some(&json!({"a": 1})));
    assert!(notification.get("id").is_none());
  }
}
