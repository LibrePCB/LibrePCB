//! Newline-delimited JSON (NDJSON) message framing for the TCP RPC
//! server.
//!
//! Every message is exactly one line, i.e. a UTF-8 encoded JSON document
//! terminated by `\n`. Since JSON strings can never contain a raw newline
//! character, a newline always terminates the message. Blank lines are
//! tolerated (and skipped) between messages.
//!
//! Both functions are transport-agnostic, i.e. they work on any
//! [BufRead]/[Write] implementation. This keeps the door open for
//! alternative transports (e.g. Unix domain sockets) without any protocol
//! changes.

use std::io::{self, BufRead, Write};

/// Maximum accepted size of a single message (1 MiB), as protection against
/// memory exhaustion caused by misbehaving clients.
const MAX_MESSAGE_SIZE: usize = 1024 * 1024;

/// Read a single message (one line) from `reader`.
///
/// Blank lines are skipped. The returned string contains the JSON document
/// without the trailing newline. Returns `Ok(None)` if the stream reached
/// EOF without a further message. Returns an error if a message exceeds
/// [MAX_MESSAGE_SIZE] or if the stream contains invalid UTF-8 — in both
/// cases the framing is broken, so the caller should drop the connection.
pub fn read_message(reader: &mut impl BufRead) -> io::Result<Option<String>> {
  let mut line = String::new();
  loop {
    let mut chunk = String::new();
    let size = reader.read_line(&mut chunk)?;
    line.push_str(&chunk);
    if line.len() > MAX_MESSAGE_SIZE {
      return Err(io::Error::new(
        io::ErrorKind::InvalidData,
        "message exceeds the maximum size",
      ));
    }
    if size == 0 {
      // EOF. A message which was not terminated by a newline is still
      // returned (and thus processed) to stay lenient.
      let message = line.trim_end().to_string();
      return Ok(if message.is_empty() {
        None
      } else {
        Some(message)
      });
    }
    if line.ends_with('\n') {
      let message = line.trim_end();
      if !message.is_empty() {
        return Ok(Some(message.to_string()));
      }
      // A blank line: Skip it and continue with the next one.
      line.clear();
    }
  }
}

/// Write a single message (JSON document + terminating newline) to
/// `writer`.
///
/// The message must not contain a newline character itself, otherwise the
/// framing would be broken.
pub fn write_message(writer: &mut impl Write, message: &str) -> io::Result<()> {
  writer.write_all(message.as_bytes())?;
  writer.write_all(b"\n")?;
  writer.flush()
}

#[cfg(test)]
mod tests {
  use super::*;
  use std::io::Cursor;

  #[test]
  fn test_read_single_message() {
    let mut cursor = Cursor::new(b"a\n".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), Some("a".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_multiple_messages() {
    let mut cursor = Cursor::new(b"one\ntwo\n".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), Some("one".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), Some("two".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_blank_lines_are_skipped() {
    let mut cursor = Cursor::new(b"\n\n \nx\n\n".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), Some("x".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_crlf() {
    let mut cursor = Cursor::new(b"x\r\n".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), Some("x".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_partial_line_at_eof() {
    // A message without trailing newline is still returned, so a lenient
    // client is not cut off.
    let mut cursor = Cursor::new(b"abc".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), Some("abc".to_string()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_empty_stream() {
    let mut cursor = Cursor::new(b"".as_slice());
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_preserves_content() {
    // Leading whitespace is preserved (JSON tolerates it), trailing
    // whitespace (including CRLF) is stripped.
    let mut cursor = Cursor::new(b"  {\"a\": 1}  \n".as_slice());
    assert_eq!(
      read_message(&mut cursor).unwrap(),
      Some("  {\"a\": 1}".to_string())
    );
  }

  #[test]
  fn test_read_invalid_utf8() {
    // The framing is broken (JSON is always valid UTF-8), so an error is
    // returned and the caller should drop the connection.
    let mut cursor = Cursor::new(vec![0xff, 0xfe, b'\n']);
    assert!(read_message(&mut cursor).is_err());
  }

  #[test]
  fn test_read_too_large() {
    // A single line exceeding the maximum size must not be buffered
    // unboundedly.
    let data = format!("{}\n", "x".repeat(MAX_MESSAGE_SIZE + 1));
    let mut cursor = Cursor::new(data.into_bytes());
    assert!(read_message(&mut cursor).is_err());
  }

  #[test]
  fn test_write_message() {
    let mut buffer = Vec::new();
    write_message(&mut buffer, r#"{"a":1}"#).unwrap();
    assert_eq!(String::from_utf8(buffer).unwrap(), "{\"a\":1}\n");
  }

  #[test]
  fn test_roundtrip() {
    let mut buffer = Vec::new();
    write_message(&mut buffer, r#"{"a":1}"#).unwrap();
    write_message(&mut buffer, r#"{"b":2}"#).unwrap();
    let mut cursor = Cursor::new(&buffer);
    assert_eq!(
      read_message(&mut cursor).unwrap(),
      Some(r#"{"a":1}"#.to_string())
    );
    assert_eq!(
      read_message(&mut cursor).unwrap(),
      Some(r#"{"b":2}"#.to_string())
    );
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }
}
