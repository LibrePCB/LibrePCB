//! FFI for [Length]
//!
//! Note that [Length] objects are passed as raw [i64] values through the FFI.
//! Basic operations (like addition/subtraction) shall be implemented purely
//! on C++ side and are thus not part of the FFI.

use crate::Length;

/// Wrapper for [Length::from_nm_f]
#[no_mangle]
extern "C" fn ffi_length_from_nm_f(val: f64, out: &mut i64) -> bool {
  if let Some(l) = Length::from_nm_f(val) {
    *out = l.to_nm();
    true
  } else {
    false
  }
}

/// Wrapper for [Length::abs]
#[no_mangle]
extern "C" fn ffi_length_abs(val: i64) -> i64 {
  Length::from_nm(val).abs().to_nm()
}

/// Wrapper for [Length::round_to]
#[no_mangle]
extern "C" fn ffi_length_round_to(val: i64, multiple: i64) -> i64 {
  Length::from_nm(val)
    .round_to(Length::from_nm(multiple))
    .to_nm()
}
