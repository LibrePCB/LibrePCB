//! FFI for base types

use std::os::raw::c_char;

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
