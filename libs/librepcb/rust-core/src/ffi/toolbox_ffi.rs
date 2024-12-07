//! FFI for [toolbox][crate::toolbox]

use super::cpp_ffi::*;
use crate::toolbox::*;

/// Wrapper for [increment_number_in_string]
#[no_mangle]
extern "C" fn ffi_increment_number_in_string(s: &mut QString) {
  let r_str = increment_number_in_string(&from_qstring(s));
  qstring_set(s, &r_str);
}
