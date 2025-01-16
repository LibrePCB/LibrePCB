//! FFI for base types

use std::io::{Result, Seek, SeekFrom, Write};
use std::os::raw::c_char;
use std::ptr;

/// cbindgen:no-export
#[doc(hidden)]
pub struct QByteArray;

#[allow(improper_ctypes)]
extern "C" {
  fn ffi_qbytearray_len(obj: &QByteArray) -> usize;
  fn ffi_qbytearray_data(obj: &QByteArray) -> *const u8;
  fn ffi_qbytearray_data_mut(obj: &mut QByteArray) -> *mut u8;
  fn ffi_qbytearray_resize(obj: &mut QByteArray, len: usize, value: u8);
}

/// Convert `QByteArray` to [`Vec<u8>`]
#[allow(dead_code)]
pub fn from_qbytearray(obj: &QByteArray) -> Vec<u8> {
  unsafe {
    let data = ffi_qbytearray_data(obj);
    let len = ffi_qbytearray_len(obj);
    let sl = std::slice::from_raw_parts(data, len);
    sl.to_vec()
  }
}

/// Get the length of a `QByteArray`
#[allow(dead_code)]
pub fn qbytearray_len(obj: &mut QByteArray) -> usize {
  unsafe { ffi_qbytearray_len(obj) }
}

/// Resize a `QByteArray`
#[allow(dead_code)]
pub fn qbytearray_resize(obj: &mut QByteArray, len: usize, value: u8) {
  unsafe {
    ffi_qbytearray_resize(obj, len, value);
  }
}

/// Convert `QByteArray` to [`&[u8]`]
#[allow(dead_code)]
pub fn qbytearray_to_slice(obj: &QByteArray) -> &[u8] {
  unsafe {
    let data = ffi_qbytearray_data(obj);
    let len = ffi_qbytearray_len(obj);
    std::slice::from_raw_parts(data, len)
  }
}

/// Convert `QByteArray` to [`&mut[u8]`]
#[allow(dead_code)]
pub fn qbytearray_to_slice_mut(obj: &mut QByteArray) -> &mut [u8] {
  unsafe {
    let data = ffi_qbytearray_data_mut(obj);
    let len = ffi_qbytearray_len(obj);
    std::slice::from_raw_parts_mut(data, len)
  }
}

/// Helper type which implements the [Write] and [Seek] traits on `QByteArray`
pub struct QByteArrayWriter {
  /// Underlying buffer
  buf: *mut QByteArray,
  /// Current cursor position
  cursor: usize,
}

impl QByteArrayWriter {
  /// Create new writer
  ///
  /// <div class="warning">
  /// The buffer lifetime must not be shorter than the lifetime of the writer!
  /// </div>
  pub fn new(buf: *mut QByteArray) -> Self {
    QByteArrayWriter { buf, cursor: 0 }
  }
}

impl Write for QByteArrayWriter {
  fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
    unsafe {
      let self_buf = self.buf.as_mut().unwrap();
      if self.cursor + buf.len() > qbytearray_len(self_buf) {
        qbytearray_resize(self_buf, self.cursor + buf.len(), 0);
      }
      let ptr = ffi_qbytearray_data_mut(self_buf);
      ptr::copy(buf.as_ptr(), ptr.add(self.cursor), buf.len());
    }
    let bytes_written = buf.len();
    self.cursor += bytes_written;
    Ok(bytes_written)
  }

  fn flush(&mut self) -> Result<()> {
    Ok(())
  }
}

impl Seek for QByteArrayWriter {
  fn seek(&mut self, pos: SeekFrom) -> std::io::Result<u64> {
    unsafe {
      let self_buf = self.buf.as_mut().unwrap();
      let self_buf_len = qbytearray_len(self_buf);
      let new_pos = match pos {
        SeekFrom::Start(offset) => offset as usize,
        SeekFrom::End(offset) => {
          let new_pos = self_buf_len.wrapping_add(offset as usize);
          if new_pos > self_buf_len {
            return Err(std::io::Error::new(
              std::io::ErrorKind::InvalidInput,
              "Seek past end of buffer",
            ));
          }
          new_pos
        }
        SeekFrom::Current(offset) => {
          let new_pos = self.cursor.wrapping_add(offset as usize);
          if new_pos > self_buf_len {
            return Err(std::io::Error::new(
              std::io::ErrorKind::InvalidInput,
              "Seek past end of buffer",
            ));
          }
          new_pos
        }
      };
      self.cursor = new_pos;
    }
    Ok(self.cursor as u64)
  }
}

/// cbindgen:no-export
#[doc(hidden)]
pub struct QString;

#[allow(improper_ctypes)]
extern "C" {
  fn ffi_qstring_len(obj: &QString) -> usize;
  fn ffi_qstring_utf16(obj: &QString) -> *const u16;
  fn ffi_qstring_set(obj: &mut QString, s: *const c_char, len: usize);
}

/// Convert `QString` to [String]
#[allow(dead_code)]
pub fn from_qstring(obj: &QString) -> String {
  unsafe {
    let data = ffi_qstring_utf16(obj);
    let len = ffi_qstring_len(obj);
    // Don't use QString::toUtf8() as it would allocate a temporary QByteArray
    // for the whole string. The approach below avoids this unnecessary copy.
    String::from_utf16_lossy(std::slice::from_raw_parts(data, len))
  }
}

/// Convert [`&str`] to `QString`
#[allow(dead_code)]
pub fn qstring_set(obj: &mut QString, s: &str) {
  unsafe { ffi_qstring_set(obj, s.as_ptr().cast(), s.len()) }
}

/// cbindgen:no-export
#[doc(hidden)]
pub struct QStringList;

#[allow(improper_ctypes)]
extern "C" {
  fn ffi_qstringlist_len(obj: &QStringList) -> usize;
  fn ffi_qstringlist_at(obj: &QStringList, index: usize) -> *const QString;
}

/// Convert `QStringList` to [`Vec<String>`]
#[allow(dead_code)]
pub fn from_qstringlist(obj: &QStringList) -> Vec<String> {
  let mut vec = Vec::new();
  unsafe {
    for i in 0..ffi_qstringlist_len(obj) {
      let s = ffi_qstringlist_at(obj, i);
      assert!(!s.is_null());
      vec.push(from_qstring(&*s));
    }
  }
  vec
}
