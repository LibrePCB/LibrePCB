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

/// Wrapper for [Length::rounded_to]
#[no_mangle]
extern "C" fn ffi_length_rounded_to(val: i64, multiple: i64) -> i64 {
  Length::from_nm(val)
    .rounded_to(Length::from_nm(multiple))
    .to_nm()
}

/// Wrapper for [Length::rounded_down_to]
#[no_mangle]
extern "C" fn ffi_length_rounded_down_to(val: i64, multiple: i64) -> i64 {
  Length::from_nm(val)
    .rounded_down_to(Length::from_nm(multiple))
    .to_nm()
}

/// Wrapper for [Length::rounded_up_to]
#[no_mangle]
extern "C" fn ffi_length_rounded_up_to(val: i64, multiple: i64) -> i64 {
  Length::from_nm(val)
    .rounded_up_to(Length::from_nm(multiple))
    .to_nm()
}
