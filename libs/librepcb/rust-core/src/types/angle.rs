//! Angle type

use std::ops::{Add, Neg, Sub};

/// Angle type
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
  /// Create 0 degrees
  pub fn deg_0() -> Self {
    Self(0)
  }

  /// Create 45 degrees
  pub fn deg_45() -> Self {
    Self(45000000)
  }

  /// Create 90 degrees
  pub fn deg_90() -> Self {
    Self(90000000)
  }

  /// Create 180 degrees
  pub fn deg_180() -> Self {
    Self(180000000)
  }

  /// Create 270 degrees
  pub fn deg_270() -> Self {
    Self(270000000)
  }

  /// Create from microdegrees
  pub fn from_udeg(val: i64) -> Self {
    Self(val % 360_000_000)
  }

  /// Convert from floating point microdegrees
  ///
  /// Rounds the value to the nearest integer and converts to [Angle] if
  /// the value is valid. For invalid values or values outside the range of
  /// [i64], [None] is returned.
  pub fn from_udeg_f(val: f64) -> Option<Self> {
    let rounded = val.round();
    if (rounded >= i64::MIN as f64) && (rounded <= i64::MAX as f64) {
      Some(Self::from_udeg(rounded as i64))
    } else {
      None
    }
  }

  /// Convert from degrees
  pub fn from_deg(val: f64) -> Option<Self> {
    Self::from_udeg_f(val * 1e6)
  }

  /// Get as migrodegrees
  pub fn to_udeg(&self) -> i64 {
    self.0
  }

  /// Convert to degrees
  pub fn to_deg(&self) -> f64 {
    self.0 as f64 / 1e6
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
  }

  #[test]
  fn predefined() {
    assert_eq!(Angle::deg_0().to_deg(), 0.0);
    assert_eq!(Angle::deg_45().to_deg(), 45.0);
    assert_eq!(Angle::deg_90().to_deg(), 90.0);
    assert_eq!(Angle::deg_180().to_deg(), 180.0);
    assert_eq!(Angle::deg_270().to_deg(), 270.0);
  }

  #[test]
  fn from_udeg() {
    assert_eq!(a(361_500_000).to_udeg(), 1_500_000);
    assert_eq!(a(-361_500_000).to_udeg(), -1_500_000);
  }

  #[test]
  fn from_to_deg() {
    let val: f64 = -42.5;
    let x = Angle::from_deg(val).unwrap();
    assert_eq!(x.to_udeg(), -42500000);
    assert_eq!(x.to_deg(), val);
  }

  #[test]
  fn from_to_deg_wrapping() {
    let val: f64 = 361.5;
    let x = Angle::from_deg(val).unwrap();
    assert_eq!(x.to_udeg(), 1500000);
    assert_eq!(x.to_deg(), 1.5);
  }
}
