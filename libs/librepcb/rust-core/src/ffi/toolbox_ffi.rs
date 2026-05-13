//! FFI for [toolbox][crate::toolbox]

use super::cpp_ffi::*;
use crate::toolbox::*;

/// Wrapper for [increment_number_in_string]
#[no_mangle]
extern "C" fn ffi_toolbox_increment_number_in_string(s: &mut QString) {
  let r_str = increment_number_in_string(&from_qstring(s));
  qstring_set(s, &r_str);
}

/// Wrapper for [lz_str::decompress_from_base64]
#[no_mangle]
extern "C" fn ffi_toolbox_decode_base64_lzstring(s: &mut QString) -> bool {
  let r_str = from_qstring(s);
  if let Some(data) = lz_str::decompress_from_base64(&r_str) {
    if let Ok(data_str) = String::from_utf16(&data) {
      qstring_set(s, &data_str);
      return true;
    }
  }
  false
}
