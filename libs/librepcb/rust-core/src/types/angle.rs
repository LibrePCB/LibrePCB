//! Angle type

use crate::math;
use std::cmp::Ordering;
use std::ops::{Add, Neg, Sub};

/// Basic type to represent an angle.
///
/// This type is used to represent *all* angle values in polygons, footprint
/// pads, boards and so on. You should never use another angle type, like
/// plain integers or floats!
///
/// All angle are stored in the integer base type [i64] in microdegrees, but
/// can be converted to other units with various methods. The range of the
/// angle is ]-360°...+360°[. So each angle (except 0 degrees) can be
/// represented in two different ways (for example +270° is equal to -90°).
/// Angles outside this range are mapped to this range (modulo), the sign will
/// be the same as before.
///
/// If you don't want an (ambiguous) angle in the range ]-360..+360[ degrees
/// but [0..360[ or [-180..+180[ degrees, there are converter methods
/// available, for example to map to 0..360 or -180..+180 degrees.
///
/// # Safety
///
/// Any overflow through arithmetic operations on this type is considered
/// undefined behavior and may panic. Make sure you never create [Angle]
/// objects with excessively large values. For calculated numbers
/// (e.g. from [f64]), make proper sanity range checks before converting
/// to [Angle].
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct Angle(i64);

impl Angle {
  /// Create an angle with 0 degrees.
  pub fn zero() -> Self {
    Self(0)
  }

  /// Create an angle from microdegrees.
  ///
  /// Maps the angle to the range ]-360°...+360°[ using modulo.
  /// The sign of the result preserves the sign of the input, so:
  /// - Positive values wrap: 361.5° → 1.5°
  /// - Negative values wrap: -361.5° → -1.5°
  pub fn from_udeg(val: i64) -> Self {
    Self(val % 360_000_000)
  }

  /// Create an angle from microdegrees (without modulo).
  ///
  /// Raw constructor without the modulo operation. Intended only for the FFI
  /// to avoid duplicated module (C++ & Rust).
  ///
  /// # Safety
  ///
  /// Shall be called only if the module has been already performed (verified
  /// with [debug_assert!]).
  pub unsafe fn from_udeg_unchecked(val: i64) -> Self {
    debug_assert!(val % 360_000_000 == val);
    Self(val)
  }

  /// Convert an angle from microdegrees (floating point).
  ///
  /// Rounds the value to the nearest integer and converts to [Angle] if
  /// the value is valid. For invalid values or values outside the range of
  /// [i64], [None] is returned.
  ///
  /// # Note
  ///
  /// The input value is *not* mapped into the range of [i64] with `fmod()`.
  /// This design decision has been made because it would still not allow us
  /// to omit the [Option] from the return type (still needed for NaN/Inf
  /// values), the `fmodf()` may add additional inaccurracies, and the case of
  /// angle values outside of [i64] can even be considered as invalid anyway.
  pub fn from_udeg_f(val: f64) -> Option<Self> {
    let rounded = val.round();
    if (rounded >= i64::MIN as f64) && (rounded <= i64::MAX as f64) {
      Some(Self::from_udeg(rounded as i64))
    } else {
      None
    }
  }

  /// Convert from degrees.
  pub fn from_deg(val: i32) -> Self {
    Self::from_udeg(val as i64 * 1_000_000)
  }

  /// Convert from degrees (floating point).
  pub fn from_deg_f(val: f64) -> Option<Self> {
    Self::from_udeg_f(val * 1e6)
  }

  /// Convert from radians (floating point).
  pub fn from_rad_f(val: f64) -> Option<Self> {
    Self::from_deg_f(math::to_degrees(val))
  }

  /// Absolute value.
  pub fn abs(&self) -> Self {
    Self(self.0.abs()) // Our value is never i64::MIN -> overflow cannot occur.
  }

  /// Round to the nearest multiple of another angle.
  ///
  /// # Safety
  ///
  /// `multiple` must be > 0 (checked by assert)!
  pub fn rounded_to(&self, multiple: Self) -> Self {
    assert!(multiple.0 > 0);
    let half = multiple.0 / 2;
    if self.0 >= 0 {
      Self((self.0 + half) / multiple.0 * multiple.0)
    } else {
      Self((self.0 - half) / multiple.0 * multiple.0)
    }
  }

  /// Inverted value.
  ///
  /// Changes the sign while keeping the represented angle. For example,
  /// 270° is converted to -90° and vice versa. As a special case, an angle
  /// of 0° will be kept as-is.
  pub fn inverted(&self) -> Self {
    match self.0.cmp(&0) {
      Ordering::Greater => Self(self.0 - 360_000_000),
      Ordering::Less => Self(self.0 + 360_000_000),
      Ordering::Equal => *self,
    }
  }

  /// Map the angle to the range [0..360[ degrees.
  pub fn to_0_360_deg(&self) -> Self {
    if self.0 < 0 {
      Self(self.0 + 360_000_000)
    } else {
      *self
    }
  }

  /// Map the angle to the range [-180..+180[ degrees.
  pub fn to_180_deg(&self) -> Self {
    if self.0 < -180_000_000 {
      Self(self.0 + 360_000_000)
    } else if self.0 >= 180_000_000 {
      Self(self.0 - 360_000_000)
    } else {
      *self
    }
  }

  /// Get as migrodegrees.
  pub fn to_udeg(&self) -> i64 {
    self.0
  }

  /// Convert to degrees.
  pub fn to_deg(&self) -> f64 {
    self.0 as f64 / 1e6
  }

  /// Convert to radians.
  pub fn to_rad(&self) -> f64 {
    math::to_radians(self.to_deg())
  }
}

impl Neg for Angle {
  type Output = Self;

  fn neg(self) -> Self {
    Self(-self.0)
  }
}

impl Add for Angle {
  type Output = Self;

  fn add(self, other: Self) -> Self {
    Self(self.0 + other.0)
  }
}

impl Sub for Angle {
  type Output = Self;

  fn sub(self, other: Self) -> Self {
    Self(self.0 - other.0)
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  fn a(udeg: i64) -> Angle {
    Angle::from_udeg(udeg)
  }

  fn af(udeg: f64) -> Option<Angle> {
    Angle::from_udeg_f(udeg)
  }

  #[test]
  fn assign() {
    #[allow(unused_assignments)]
    let mut x = a(10);
    let mut y = a(20);
    x = y;
    y = a(30);
    assert_eq!(x, a(20));
    assert_eq!(y, a(30));
  }

  #[test]
  fn compare() {
    assert_eq!(a(1) == a(1), true);
    assert_eq!(a(1) == a(2), false);
    assert_eq!(a(1) != a(2), true);
    assert_eq!(a(1) != a(1), false);
  }

  #[test]
  fn arithmetic() {
    assert_eq!(a(-5) + a(10), a(5));
    assert_eq!(a(5) - a(10), a(-5));
    assert_eq!(-a(5), a(-5));
    assert_eq!(-a(-5), a(5));
  }

  #[test]
  fn zero() {
    assert_eq!(Angle::zero().to_udeg(), 0);
  }

  #[test]
  fn from_to_udeg() {
    assert_eq!(a(359_999_999).to_udeg(), 359_999_999);
    assert_eq!(a(-359_999_999).to_udeg(), -359_999_999);
    assert_eq!(a(360_000_000).to_udeg(), 0);
    assert_eq!(a(-360_000_000).to_udeg(), 0);
    assert_eq!(a(361_500_000).to_udeg(), 1_500_000);
    assert_eq!(a(-361_500_000).to_udeg(), -1_500_000);
  }

  #[test]
  fn from_to_udeg_unchecked() {
    unsafe {
      assert_eq!(Angle::from_udeg_unchecked(0).to_udeg(), 0);
    }
  }

  #[test]
  fn from_to_udeg_f() {
    assert_eq!(af(std::f64::NAN), None);
    assert_eq!(af(std::f64::INFINITY), None);
    assert_eq!(af(std::f64::NEG_INFINITY), None);
    assert_eq!(af(361000000.7).unwrap().to_udeg(), 1_000_001);
    assert_eq!(af(361000000.3).unwrap().to_udeg(), 1_000_000);
    assert_eq!(af(-361000000.7).unwrap().to_udeg(), -1_000_001);
    assert_eq!(af(-361000000.3).unwrap().to_udeg(), -1_000_000);
  }

  #[test]
  fn from_to_deg() {
    let val: i32 = -42;
    let x = Angle::from_deg(val);
    assert_eq!(x.to_udeg(), -42000000);
    assert_eq!(x.to_deg(), val as f64);
  }

  #[test]
  fn from_to_deg_f_nan() {
    assert_eq!(Angle::from_deg_f(std::f64::NAN), None);
  }

  #[test]
  fn from_to_deg_f() {
    let val: f64 = -42.5;
    let x = Angle::from_deg_f(val).unwrap();
    assert_eq!(x.to_udeg(), -42500000);
    assert_eq!(x.to_deg(), val);
  }

  #[test]
  fn from_to_deg_f_wrapping() {
    let val: f64 = 361.5;
    let x = Angle::from_deg_f(val).unwrap();
    assert_eq!(x.to_udeg(), 1500000);
    assert_eq!(x.to_deg(), 1.5);
  }

  #[test]
  fn from_to_deg_f_wrapping_negative() {
    let val: f64 = -361.5;
    let x = Angle::from_deg_f(val).unwrap();
    assert_eq!(x.to_udeg(), -1500000);
    assert_eq!(x.to_deg(), -1.5);
  }

  #[test]
  fn from_to_rad_f_nan() {
    assert_eq!(Angle::from_rad_f(std::f64::NAN), None);
  }

  #[test]
  fn from_to_rad_f() {
    let val: f64 = std::f64::consts::PI;
    let x = Angle::from_rad_f(val).unwrap();
    assert_eq!(x.to_udeg(), 180_000_000);
    assert_eq!(x.to_rad(), val);
  }

  #[test]
  fn from_to_rad_f_negative() {
    let val: f64 = std::f64::consts::PI / -2.0;
    let x = Angle::from_rad_f(val).unwrap();
    assert_eq!(x.to_udeg(), -90_000_000);
    assert_eq!(x.to_rad(), val);
  }

  #[test]
  fn abs() {
    assert_eq!(a(0).abs().to_udeg(), 0);
    assert_eq!(a(180_000_000).abs().to_udeg(), 180_000_000);
    assert_eq!(a(-180_000_000).abs().to_udeg(), 180_000_000);
    assert_eq!(a(-359_999_999).abs().to_udeg(), 359_999_999);
  }

  create! {rounded_to, (input, multiple, rounded), {
    assert_eq!(a(input).rounded_to(a(multiple)), a(rounded));
  }}
  rounded_to! {
    d01: (  0, 10,   0),
    d02: ( 10,  1,  10),
    d03: (-10,  1, -10),
    d04: (  8, 10,  10),
    d05: (  2, 10,   0),
    d06: ( -8, 10, -10),
    d07: ( -2, 10,   0),
    d08: ( 18, 10,  20),
    d09: ( 12, 10,  10),
    d10: (-18, 10, -20),
    d11: (-12, 10, -10),
    d12: ( 10, 10,  10),
    d13: (-10, 10, -10),
    d14: ( 20, 10,  20),
    d15: (-20, 10, -20),
    d16: ( 15, 10,  20),
    d17: (-15, 10, -20),
  }

  #[test]
  fn inverted() {
    assert_eq!(a(0).inverted().to_udeg(), 0);
    assert_eq!(a(180_000_000).inverted().to_udeg(), -180_000_000);
    assert_eq!(a(-180_000_000).inverted().to_udeg(), 180_000_000);
    assert_eq!(a(-90_000_000).inverted().to_udeg(), 270_000_000);
    assert_eq!(a(90_000_000).inverted().to_udeg(), -270_000_000);
    assert_eq!(a(-270_000_000).inverted().to_udeg(), 90_000_000);
    assert_eq!(a(270_000_000).inverted().to_udeg(), -90_000_000);
  }

  #[test]
  fn to_0_360_deg() {
    assert_eq!(a(0).to_0_360_deg().to_udeg(), 0);
    assert_eq!(a(-90_000_000).to_0_360_deg().to_udeg(), 270_000_000);
    assert_eq!(a(-180_000_000).to_0_360_deg().to_udeg(), 180_000_000);
    assert_eq!(a(90_000_000).to_0_360_deg().to_udeg(), 90_000_000);
    assert_eq!(a(180_000_000).to_0_360_deg().to_udeg(), 180_000_000);
  }

  #[test]
  fn to_180_deg() {
    assert_eq!(a(0).to_180_deg().to_udeg(), 0);
    assert_eq!(a(-90_000_000).to_180_deg().to_udeg(), -90_000_000);
    assert_eq!(a(-180_000_000).to_180_deg().to_udeg(), -180_000_000);
    assert_eq!(a(-270_000_000).to_180_deg().to_udeg(), 90_000_000);
    assert_eq!(a(90_000_000).to_180_deg().to_udeg(), 90_000_000);
    assert_eq!(a(180_000_000).to_180_deg().to_udeg(), -180_000_000);
    assert_eq!(a(270_000_000).to_180_deg().to_udeg(), -90_000_000);
  }
}
