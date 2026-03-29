//! FFI for [math][crate::math]

use crate::math::*;

/// Wrapper for [rotate_point]
#[no_mangle]
extern "C" fn ffi_math_rotate_point(x: &mut f64, y: &mut f64, angle: f64) {
  (*x, *y) = rotate_point(*x, *y, angle)
}

/// Wrapper for [angle_to_point]
#[no_mangle]
extern "C" fn ffi_math_angle_to_point(x: f64, y: f64) -> f64 {
  angle_to_point(x, y)
}

/// Wrapper for [arc_radius]
#[no_mangle]
extern "C" fn ffi_math_arc_radius(dx: f64, dy: f64, angle: f64) -> f64 {
  arc_radius(dx, dy, angle)
}

/// Wrapper for [arc_radius_and_center]
#[no_mangle]
extern "C" fn ffi_math_arc_radius_and_center(
  dx: f64,
  dy: f64,
  angle: f64,
  x: &mut f64,
  y: &mut f64,
) -> f64 {
  let (radius, center) = arc_radius_and_center(dx, dy, angle);
  (*x, *y) = center;
  radius
}
