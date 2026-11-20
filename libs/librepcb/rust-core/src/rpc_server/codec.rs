//! MessagePack stream framing for the RPC server.
//!
//! Messages are msgpack values decoded *directly from the stream* (i.e.
//! without any length prefix), just like the official msgpack-rpc protocol:
//! msgpack is self-delimiting, so the decoder knows exactly where each
//! value ends. A decoder which consumes the stream byte by byte (like
//! `rmpv::decode::read_value`) never needs to re-parse partial data — it
//! simply blocks until more bytes arrive.
//!
//! The size of a single message is limited (protection against memory
//! exhaustion caused by misbehaving clients). Since the size is not known
//! before parsing, the limit is enforced *mid-parse* by a counting reader:
//! once it has consumed more than [MAX_MESSAGE_SIZE] bytes for a single
//! value, it errors, which aborts the decoding.
//!
//! Both functions are transport-agnostic, i.e. they work on any
//! [BufRead]/[Write] implementation. This keeps the door open for
//! alternative transports (e.g. WebSocket or Unix domain sockets) without
//! any protocol changes.

use std::io::{self, BufRead, Read};

use rmpv::Value;

/// Maximum accepted size of a single message (32 MiB), as protection
/// against memory exhaustion caused by misbehaving clients. Enforced
/// mid-parse (see module documentation).
pub const MAX_MESSAGE_SIZE: usize = 32 * 1024 * 1024;

/// Read a single message (one msgpack value) from `reader`.
///
/// Returns `Ok(None)` if the stream reached EOF cleanly *between* messages.
/// Returns an error if the stream contains invalid msgpack, if a message
/// is truncated (EOF in the middle of a value) or if it exceeds
/// [MAX_MESSAGE_SIZE] — in all these cases the framing is broken (the
/// decoder's stream position is no longer trustworthy), so the caller must
/// drop the connection.
///
/// The reader blocks until a complete message was received (partial
/// messages are *not* re-parsed, see module documentation). A message
/// which is never completed (e.g. a client which sends a truncated value
/// and then stalls) blocks this reader until the stream reaches EOF —
/// that's inherent to stream framing, and harmless since it only affects
/// that one connection.
pub fn read_message(reader: &mut impl BufRead) -> io::Result<Option<Value>> {
  // A clean EOF is only valid between messages, so peek one byte first.
  if reader.fill_buf()?.is_empty() {
    return Ok(None);
  }
  let mut counting = CountingReader::new(reader);
  match rmpv::decode::read_value(&mut counting) {
    Ok(value) => Ok(Some(value)),
    Err(e) => Err(io::Error::new(io::ErrorKind::InvalidData, e.to_string())),
  }
}

/// Encode a message (one msgpack value) to bytes, ready to be written to
/// the stream.
pub fn encode_message(value: &Value) -> io::Result<Vec<u8>> {
  let mut buffer = Vec::new();
  rmpv::encode::write_value(&mut buffer, value)
    .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e.to_string()))?;
  Ok(buffer)
}

/// A reader which counts the bytes consumed by the decoder and errors once
/// more than [MAX_MESSAGE_SIZE] bytes were read for a single message.
///
/// It is implemented on top of a [BufRead] (using `fill_buf`/`consume`) so
/// that exactly the bytes *consumed by the decoder* are counted — not the
/// bytes which the buffering layer may have read ahead.
struct CountingReader<'a, R: BufRead + ?Sized> {
  /// The underlying buffered reader.
  inner: &'a mut R,
  /// Number of bytes consumed so far (for the current message).
  consumed: usize,
}

impl<'a, R: BufRead + ?Sized> CountingReader<'a, R> {
  /// Create a new counting reader.
  fn new(inner: &'a mut R) -> Self {
    Self { inner, consumed: 0 }
  }
}

impl<R: BufRead + ?Sized> Read for CountingReader<'_, R> {
  fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
    let available = self.inner.fill_buf()?;
    if available.is_empty() {
      return Ok(0); // EOF
    }
    let n = buf.len().min(available.len());
    buf[..n].copy_from_slice(&available[..n]);
    self.inner.consume(n);
    self.consumed += n;
    if self.consumed > MAX_MESSAGE_SIZE {
      return Err(io::Error::new(
        io::ErrorKind::InvalidData,
        "message exceeds the maximum size",
      ));
    }
    Ok(n)
  }
}

#[cfg(test)]
mod tests {
  use super::*;
  use std::io::Cursor;

  /// Encode a value and decode it back, asserting equality.
  fn roundtrip(value: &Value) {
    let bytes = encode_message(value).unwrap();
    let mut cursor = Cursor::new(bytes);
    assert_eq!(read_message(&mut cursor).unwrap(), Some(value.clone()));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_roundtrip_scalars() {
    roundtrip(&Value::Nil);
    roundtrip(&Value::from(true));
    roundtrip(&Value::from(false));
    roundtrip(&Value::from(42i64));
    roundtrip(&Value::from(-1i64));
    roundtrip(&Value::from(1.5f64));
    roundtrip(&Value::from("hello"));
    roundtrip(&Value::Binary(vec![1, 2, 3]));
  }

  #[test]
  fn test_roundtrip_containers() {
    roundtrip(&Value::Array(vec![Value::from(1), Value::from("x")]));
    roundtrip(&Value::Map(vec![
      (Value::from("a"), Value::from(1)),
      (Value::from("b"), Value::Array(vec![Value::Nil])),
    ]));
  }

  #[test]
  fn test_read_multiple_messages() {
    let mut buffer = Vec::new();
    buffer.extend(encode_message(&Value::from(1)).unwrap());
    buffer.extend(encode_message(&Value::from("two")).unwrap());
    let mut cursor = Cursor::new(buffer);
    assert_eq!(read_message(&mut cursor).unwrap(), Some(Value::from(1)));
    assert_eq!(read_message(&mut cursor).unwrap(), Some(Value::from("two")));
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_empty_stream() {
    let mut cursor = Cursor::new(Vec::new());
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }

  #[test]
  fn test_read_partial_message_across_chunks() {
    // A message split into two chunks is decoded once the rest arrives —
    // the decoder blocks mid-value instead of failing (or re-parsing).
    let bytes = encode_message(&Value::Array(vec![
      Value::from(1),
      Value::from(2),
      Value::from(3),
    ]))
    .unwrap();
    let (first, second) = bytes.split_at(bytes.len() / 2);
    let mut reader =
      ChunkedBufReader::new(vec![first.to_vec(), second.to_vec()]);
    let value = read_message(&mut reader).unwrap();
    assert_eq!(
      value,
      Some(Value::Array(vec![
        Value::from(1),
        Value::from(2),
        Value::from(3)
      ]))
    );
    assert_eq!(read_message(&mut reader).unwrap(), None);
  }

  /// A [BufRead] which returns exactly one chunk at a time (i.e. it
  /// emulates a socket where the data arrives in multiple parts).
  struct ChunkedBufReader {
    /// Pending chunks (the first one is `current`).
    pending: Vec<Vec<u8>>,
    /// The currently readable chunk.
    current: Vec<u8>,
  }

  impl ChunkedBufReader {
    fn new(mut chunks: Vec<Vec<u8>>) -> Self {
      let current = if chunks.is_empty() {
        Vec::new()
      } else {
        chunks.remove(0)
      };
      Self {
        pending: chunks,
        current,
      }
    }
  }

  impl Read for ChunkedBufReader {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
      let available = self.fill_buf()?;
      let n = buf.len().min(available.len());
      buf[..n].copy_from_slice(&available[..n]);
      self.consume(n);
      Ok(n)
    }
  }

  impl BufRead for ChunkedBufReader {
    fn fill_buf(&mut self) -> io::Result<&[u8]> {
      if self.current.is_empty() && !self.pending.is_empty() {
        self.current = self.pending.remove(0);
      }
      Ok(&self.current)
    }

    fn consume(&mut self, amt: usize) {
      assert!(amt <= self.current.len());
      self.current = self.current[amt..].to_vec();
    }
  }

  #[test]
  fn test_read_invalid_msgpack() {
    // An ext8 value announcing 5 data bytes, but only 1 following.
    let mut cursor = Cursor::new(vec![0xc7, 0x05, 0x01]);
    assert!(read_message(&mut cursor).is_err());
    // A str8 value announcing 16 bytes, but none following.
    let mut cursor = Cursor::new(vec![0xd9, 0x10]);
    assert!(read_message(&mut cursor).is_err());
  }

  #[test]
  fn test_read_truncated_message() {
    // An array marker announcing 3 elements, but only 1 byte following.
    let mut cursor = Cursor::new(vec![0x93, 0x01]);
    assert!(read_message(&mut cursor).is_err());
  }

  #[test]
  fn test_read_too_large() {
    // A map announcing far more entries than allowed (the cap hits
    // mid-parse, long before the announced size is buffered).
    let mut data = vec![0xdf]; // map32
    data.extend(0xffff_ffffu32.to_be_bytes());
    let mut cursor = Cursor::new(data);
    assert!(read_message(&mut cursor).is_err());
  }

  #[test]
  fn test_decode_error_leaves_stream_desynced() {
    // This test documents why decode errors must close the connection: A
    // str8 value announcing 5 bytes but delivering only 3 makes the
    // decoder consume the following messages as string content, i.e. the
    // stream is desynchronized and the subsequent messages (two encoded
    // integers) silently vanish as string content.
    let mut buffer = vec![0xd9, 0x05, b'h', b'e', b'l'];
    buffer.extend(encode_message(&Value::from(1)).unwrap());
    buffer.extend(encode_message(&Value::from(1)).unwrap());
    let mut cursor = Cursor::new(buffer);
    // The "message" decodes to a garbage string which swallowed the rest:
    assert_eq!(
      read_message(&mut cursor).unwrap(),
      Some(Value::from("hel\u{1}\u{1}"))
    );
    // The two real messages are gone, the stream is at EOF:
    assert_eq!(read_message(&mut cursor).unwrap(), None);
  }
}
