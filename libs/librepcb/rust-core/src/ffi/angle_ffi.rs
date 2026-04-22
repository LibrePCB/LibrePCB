//! FFI for [Angle]
//!
//! Note that [Angle] objects are passed as raw [i64] values through the FFI.
//! Basic operations (like addition/subtraction) shall be implemented purely
//! on C++ side and are thus not part of the FFI.

use crate::Angle;

/// Wrapper for [Angle::from_udeg_f]
#[no_mangle]
extern "C" fn ffi_angle_from_udeg_f(val: f64, out: &mut i64) -> bool {
  if let Some(l) = Angle::from_udeg_f(val) {
    *out = l.to_udeg();
    true
  } else {
    false
  }
}

/// Wrapper for [Angle::from_deg_f]
#[no_mangle]
extern "C" fn ffi_angle_from_deg_f(val: f64, out: &mut i64) -> bool {
  if let Some(l) = Angle::from_deg_f(val) {
    *out = l.to_udeg();
    true
  } else {
    false
  }
}

/// Wrapper for [Angle::from_rad_f]
#[no_mangle]
extern "C" fn ffi_angle_from_rad_f(val: f64, out: &mut i64) -> bool {
  if let Some(l) = Angle::from_rad_f(val) {
    *out = l.to_udeg();
    true
  } else {
    false
  }
}

/// Wrapper for [Angle::abs]
#[no_mangle]
unsafe extern "C" fn ffi_angle_abs(val: i64) -> i64 {
  Angle::from_udeg_unchecked(val).abs().to_udeg()
}

/// Wrapper for [Angle::rounded_to]
#[no_mangle]
unsafe extern "C" fn ffi_angle_rounded_to(val: i64, multiple: i64) -> i64 {
  Angle::from_udeg_unchecked(val)
    .rounded_to(Angle::from_udeg_unchecked(multiple))
    .to_udeg()
}

/// Wrapper for [Angle::inverted]
#[no_mangle]
unsafe extern "C" fn ffi_angle_inverted(val: i64) -> i64 {
  Angle::from_udeg_unchecked(val).inverted().to_udeg()
}

/// Wrapper for [Angle::to_0_360_deg]
#[no_mangle]
unsafe extern "C" fn ffi_angle_to_0_360_deg(val: i64) -> i64 {
  Angle::from_udeg_unchecked(val).to_0_360_deg().to_udeg()
}

/// Wrapper for [Angle::to_180_deg]
#[no_mangle]
unsafe extern "C" fn ffi_angle_to_180_deg(val: i64) -> i64 {
  Angle::from_udeg_unchecked(val).to_180_deg().to_udeg()
}
