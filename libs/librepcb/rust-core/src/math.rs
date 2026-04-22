//! Various low-level math functions.
//!
//! <div class="warning">
//! All functions in this module are deterministic across all platforms.
//! This is mainly done by using floating point math from the
//! <a href="https://docs.rs/libm/latest/libm/">libm</a> library.
//! </div>

use libm;

/// π/180 as a deterministic constant.
const PI_DIV_180: f64 = f64::from_bits(0x3F91DF46A2529D39);

/// Convert degrees to radians.
pub fn to_radians(degrees: f64) -> f64 {
  degrees * PI_DIV_180
}

/// Convert radians to degrees.
pub fn to_degrees(radians: f64) -> f64 {
  radians / PI_DIV_180
}

/// Rotate a point (x, y) by a certain angle around (0, 0).
///
/// # Notes
///
/// This is a low-level function that always rotates the point using floating-
/// point math, even for angles which are a multiple of 90 degrees. It is
/// recommended to handle those easy cases explicitly, and only call this
/// function for the non-obvious cases.
///
/// # ATTENTION
///
/// The passed angle has to be in the range ]-360, 360[, otherwise
/// an assert is raised.
///
/// # Arguments
///
/// * `x` - X-coordiante of the point.
/// * `y` - Y-coordiante of the point.
/// * `angle` - Angle in degrees counter-clockwise.
///
/// # Returns
///
/// Returns the rotated point as (X, Y) tuple.
pub fn rotate_point(x: f64, y: f64, angle: f64) -> (f64, f64) {
  debug_assert!((angle > -360.0) && (angle < 360.0));
  let radians = to_radians(angle);
  let sin = libm::sin(radians);
  let cos = libm::cos(radians);
  (cos * x - sin * y, sin * x + cos * y)
}

/// Calculate the angle from (0, 0) to a given point (x, y).
///
/// # Notes
///
/// This is a low-level function that always calculates the angle using
/// floating-point math, even for angles which are a multiple of 90 degrees.
/// It is recommended to handle those easy cases explicitly, and only call this
/// function for the non-obvious cases.
///
/// # Arguments
///
/// * `x` - X-coordiante of the point.
/// * `y` - Y-coordiante of the point.
///
/// # Returns
///
/// Returns the angle in degrees (east=0, north=90, west=180, south=-90).
pub fn angle_to_point(x: f64, y: f64) -> f64 {
  to_degrees(libm::atan2(y, x))
}

/// Calculate the radius of an arc given by start/end point and angle.
///
/// The arc is given by its start point, end point and angle. The start point
/// is always (0, 0).
///
/// # ATTENTION
///
/// This is a low-level function that doesn't handle invalid cases and may
/// lead to NaN or infinite output depending on the given input. Always check
/// the returned value for validity.
///
/// In addition, the passed angle has to be in the range ]-360, 360[, otherwise
/// an assert is raised.
///
/// # Arguments
///
/// * `x` - X-coordiante of the arcs end point.
/// * `y` - Y-coordiante of the arcs end point.
/// * `angle` - Arc angle in degrees counter-clockwise.
///
/// # Returns
///
/// Returns the arc radius (may be NaN or infinite).
pub fn arc_radius(dx: f64, dy: f64, angle: f64) -> f64 {
  debug_assert!((angle > -360.0) && (angle < 360.0));
  let d = libm::sqrt(dx * dx + dy * dy);
  d / (2.0 * libm::sin(to_radians(angle / 2.0)))
}

/// Calculate the radius and center of an arc given by start/end point and
/// angle.
///
/// The arc is given by its start point, end point and angle. The start point
/// is always (0, 0). The algorithm is described in
/// <https://math.stackexchange.com/questions/27535/>.
///
/// # ATTENTION
///
/// This is a low-level function that doesn't handle invalid cases and may
/// lead to NaN or infinite output depending on the given input. Always check
/// the returned value for validity.
///
/// In addition, the passed angle has to be in the range ]-360, 360[, otherwise
/// an assert is raised.
///
/// # Arguments
///
/// * `x` - X-coordiante of the arcs end point.
/// * `y` - Y-coordiante of the arcs end point.
/// * `angle` - Arc angle in degrees counter-clockwise.
///
/// # Returns
///
/// Returns the arc radius and center (both may be NaN or infinite).
pub fn arc_radius_and_center(
  dx: f64,
  dy: f64,
  angle: f64,
) -> (f64, (f64, f64)) {
  debug_assert!((angle > -360.0) && (angle < 360.0));
  let d = libm::sqrt(dx * dx + dy * dy);
  let r = d / (2.0 * libm::sin(to_radians(angle / 2.0)));
  let mut h0 = r * r - d * d / 4.0;
  if h0 <= 0.0 {
    h0 = 0.0; // Fixes https://github.com/LibrePCB/LibrePCB/issues/974.
  }
  let h = libm::sqrt(h0);
  let u = dx / d;
  let v = dy / d;
  let angle_sgn = if angle >= 0.0 { 1.0 } else { -1.0 };
  let x = (dx / 2.0) - h * v * angle_sgn;
  let y = (dy / 2.0) + h * u * angle_sgn;
  (r, (x, y))
}

#[cfg(test)]
mod tests {
  use super::*;

  macro_rules! assert_eq_nan {
    ($left:expr, $right:expr $(,)?) => {
      match (&$left, &$right) {
        (l, r) if l.is_nan() && r.is_nan() => {}
        (l, r) => assert_eq!(l, r),
      }
    };
  }

  // Zero returned by libm::sin(x) or libm::cos(x)
  const ZERO1: f64 = 6.123233995736766e-16;
  // Zero returned by libm::sin(x) or libm::cos(x)
  const ZERO2: f64 = 1.2246467991473533e-15;
  // Ten returned by 10 + libm::sin(x) or libm::cos(x)
  const TEN1: f64 = 9.999999999999998;
  // Ten returned by 10 + libm::sin(x) or libm::cos(x)
  const TEN2: f64 = 10.000000000000002;

  create! {rotate_point, (x, y, angle, expected), {
    assert_eq!(rotate_point(x, y, angle), expected);
  }}
  rotate_point! {
    //       x,    y,  angle, (out_x, out_y)
    d01: ( 0.0,  0.0,  -90.0, (  0.0,   0.0)),
    d02: ( 0.0,  0.0,    0.0, (  0.0,   0.0)),
    d03: ( 0.0,  0.0,   90.0, (  0.0,   0.0)),
    d04: ( 0.0,  0.0,  180.0, (  0.0,   0.0)),

    d10: (10.0,  0.0,  -90.0, (ZERO1, -10.0)),
    d11: (10.0,  0.0,    0.0, ( 10.0,   0.0)),
    d12: (10.0,  0.0,   90.0, (ZERO1,  10.0)),
    d13: (10.0,  0.0,  180.0, (-10.0, ZERO2)),

    d20: (10.0, 20.0,  -90.0, ( 20.0, -TEN1)),
    d21: (10.0, 20.0,    0.0, ( 10.0,  20.0)),
    d22: (10.0, 20.0,   90.0, (-20.0,  TEN2)),
    d23: (10.0, 20.0,  180.0, (-TEN2, -20.0)),

    d30: (10.0,  0.0,    1.0, (9.9984769515639120, 0.17452406437283513)),
    d31: (10.0,  0.0,    2.0, (9.9939082701909570, 0.34899496702500970)),
    d32: (10.0,  0.0,   45.0, (7.0710678118654755, 7.07106781186547500)),
  }

  create! {angle_to_point, (x, y, expected), {
    assert_eq_nan!(angle_to_point(x, y), expected);
  }}
  angle_to_point! {
    //        x,     y,      out
    d01: (  0.0,   0.0,   0.0f64),
    d02: ( 10.0,   0.0,   0.0f64),
    d03: (  0.0,  10.0,  90.0f64),
    d04: (-10.0,   0.0, 180.0f64),
    d05: (  0.0, -10.0, -90.0f64),
  }

  create! {arc_radius_and_center, (x, y, angle, expected), {
    let actual_r1 = arc_radius(x, y, angle);
    let (actual_r2, (actual_x, actual_y)) = arc_radius_and_center(x, y, angle);
    assert_eq_nan!(actual_r1, expected.0);
    assert_eq_nan!(actual_r2, expected.0);
    assert_eq_nan!(actual_x, expected.1.0);
    assert_eq_nan!(actual_y, expected.1.1);
  }}
  arc_radius_and_center! {
    //       x,               y, angle, (        out_r, (         out_x,          out_y))
    d01: ( 0.0,             0.0,   0.0, (     f64::NAN, (      f64::NAN,      f64::NAN))),
    d02: ( 0.0,             0.0,  90.0, (       0.0f64, (      f64::NAN,      f64::NAN))),
    d03: (10.0,            20.0,   0.0, (f64::INFINITY, (-f64::INFINITY, f64::INFINITY))),
    d10: (762.0,     -1319.8227, -90.0, (-1077.6307251176745f64, (-278.91134999999997f64, -1040.9113499999999f64))),
    // Test to reproduce https://github.com/LibrePCB/LibrePCB/issues/974
    d11: (-46.0,            0.0, 180.0, (23.00000f64, (-23.00000f64, 0.0f64))),
    // Test to reproduce another case where small deviations were observed
    d12: (-1813077.0, 1246093.0, 180.0, (1099999.542338314f64, (-906538.5f64, 623046.5f64))),
  }
}
