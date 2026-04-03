//! Length type

use std::ops::{Add, Sub};

/// Nanometers per inch
const NM_PER_INCH: f64 = 25400000.0;
/// Nanometers per mil
const NM_PER_MIL: f64 = 25400.0;
/// Pixels per inch (dpi)
const PIXELS_PER_INCH: f64 = 72.0;
/// Nanometers per pixel
const NM_PER_PIXEL: f64 = NM_PER_INCH / PIXELS_PER_INCH;

/// Length type
///
/// This type is used to represent ALL length values in symbols, schematics,
/// footprints, boards and so on. You should never use another length type,
/// like plain integers or floats!
///
/// All lengths are stored in the integer base type [i64] in nanometers, but
/// can be converted to other units with various methods.
///
/// # Safety
///
/// Any overflow through arithmetic operations on this type is considered
/// undefined behavior and may panic. Make sure you never create [Length]
/// objects with excessively large values. For calculated numbers
/// (e.g. from [f64]), make proper sanity range checks before converting
/// to [Length].
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Length(i64);

impl Length {
  /// Create from nanometers
  pub fn from_nm(val: i64) -> Self {
    Self(val)
  }

  /// Convert from floating point nanometers
  ///
  /// Rounds the value to the nearest integer and converts to [Length] if
  /// the value is valid. For invalid values or values outside the range of
  /// [i64], [None] is returned.
  pub fn from_nm_f(val: f64) -> Option<Self> {
    let rounded = val.round();
    if (rounded >= i64::MIN as f64) && (rounded <= i64::MAX as f64) {
      Some(Self(rounded as i64))
    } else {
      None
    }
  }

  /// Convert from micrometers
  pub fn from_um(val: f64) -> Option<Self> {
    Self::from_nm_f(val * 1e3)
  }

  /// Convert from millimeters
  pub fn from_mm(val: f64) -> Option<Self> {
    Self::from_nm_f(val * 1e6)
  }

  /// Convert from inches
  pub fn from_inch(val: f64) -> Option<Self> {
    Self::from_nm_f(val * NM_PER_INCH)
  }

  /// Convert from mils
  pub fn from_mils(val: f64) -> Option<Self> {
    Self::from_nm_f(val * NM_PER_MIL)
  }

  /// Convert from pixels
  pub fn from_px(val: f64) -> Option<Self> {
    Self::from_nm_f(val * NM_PER_PIXEL)
  }

  /// Absolute value
  pub fn abs(&self) -> Self {
    Self(self.0.saturating_abs())
  }

  /// Round to the nearest multiple of another length
  ///
  /// Useful to map coordinates to a grid.
  ///
  /// # Safety
  ///
  /// `multiple` must be > 0 (checked by assert)!
  pub fn rounded_to(&self, multiple: Length) -> Self {
    assert!(multiple.0 > 0);
    let half = multiple.0 / 2;
    if self.0 >= 0 {
      Self((self.0 + half) / multiple.0 * multiple.0)
    } else {
      Self((self.0 - half) / multiple.0 * multiple.0)
    }
  }

  /// Round to the next lower (or equal) multiple of another length
  ///
  /// # Safety
  ///
  /// `multiple` must be > 0 (checked by assert)!
  pub fn rounded_down_to(&self, multiple: Length) -> Self {
    let rounded = self.rounded_to(multiple);
    if rounded > *self {
      rounded - multiple
    } else {
      rounded
    }
  }

  /// Round to the next higher (or equal) multiple of another length
  ///
  /// # Safety
  ///
  /// `multiple` must be > 0 (checked by assert)!
  pub fn rounded_up_to(&self, multiple: Length) -> Self {
    let rounded = self.rounded_to(multiple);
    if rounded < *self {
      rounded + multiple
    } else {
      rounded
    }
  }

  /// Get as nanometers
  pub fn to_nm(&self) -> i64 {
    self.0
  }

  /// Convert to micrometers
  pub fn to_um(&self) -> f64 {
    self.0 as f64 / 1e3
  }

  /// Convert to millimeters
  pub fn to_mm(&self) -> f64 {
    self.0 as f64 / 1e6
  }

  /// Convert to inches
  pub fn to_inch(&self) -> f64 {
    self.0 as f64 / NM_PER_INCH
  }

  /// Convert to mils
  pub fn to_mils(&self) -> f64 {
    self.0 as f64 / NM_PER_MIL
  }

  /// Convert to pixels
  pub fn to_px(&self) -> f64 {
    self.0 as f64 / NM_PER_PIXEL
  }
}

impl Add for Length {
  type Output = Self;

  fn add(self, other: Self) -> Self {
    Self(self.0 + other.0)
  }
}

impl Sub for Length {
  type Output = Self;

  fn sub(self, other: Self) -> Self {
    Self(self.0 - other.0)
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  fn l(nm: i64) -> Length {
    Length::from_nm(nm)
  }

  #[test]
  fn assign() {
    #[allow(unused_assignments)]
    let mut a = l(10);
    let mut b = l(20);
    a = b;
    b = l(30);
    assert_eq!(a, l(20));
    assert_eq!(b, l(30));
  }

  #[test]
  fn compare() {
    assert_eq!(l(1) == l(1), true);
    assert_eq!(l(1) == l(2), false);
    assert_eq!(l(1) != l(2), true);
    assert_eq!(l(1) != l(1), false);
    assert_eq!(l(2) >= l(1), true);
    assert_eq!(l(2) >= l(2), true);
    assert_eq!(l(2) >= l(3), false);
    assert_eq!(l(2) <= l(3), true);
    assert_eq!(l(2) <= l(2), true);
    assert_eq!(l(2) <= l(1), false);
    assert_eq!(l(2) > l(1), true);
    assert_eq!(l(2) > l(2), false);
    assert_eq!(l(2) > l(3), false);
    assert_eq!(l(2) < l(3), true);
    assert_eq!(l(2) < l(2), false);
    assert_eq!(l(2) < l(1), false);
  }

  #[test]
  fn arithmetic() {
    assert_eq!(l(-5) + l(10), l(5));
    assert_eq!(l(5) - l(10), l(-5));
  }

  #[test]
  fn from_to_um() {
    let val: f64 = -42.5;
    let x = Length::from_um(val).unwrap();
    assert_eq!(x.to_nm(), -42500);
    assert_eq!(x.to_um(), val);
  }

  #[test]
  fn from_to_mm() {
    let val: f64 = -42.5;
    let x = Length::from_mm(val).unwrap();
    assert_eq!(x.to_nm(), -42500000);
    assert_eq!(x.to_mm(), val);
  }

  #[test]
  fn from_to_inch() {
    let val: f64 = -42.5;
    let x = Length::from_inch(val).unwrap();
    assert_eq!(x.to_nm(), -1079500000);
    assert_eq!(x.to_inch(), val);
  }

  #[test]
  fn from_to_mils() {
    let val: f64 = -42.5;
    let x = Length::from_mils(val).unwrap();
    assert_eq!(x.to_nm(), -1079500);
    assert_eq!(x.to_mils(), val);
  }

  #[test]
  fn from_to_px() {
    let val: f64 = -4.5;
    let x = Length::from_px(val).unwrap();
    assert_eq!(x.to_nm(), -1587500);
    assert_eq!(x.to_px(), val);
  }

  create! {rounded_to, (input, multiple, rounded, lower, upper), {
    assert_eq!(l(input).rounded_to(l(multiple)), l(rounded));
    assert_eq!(l(input).rounded_down_to(l(multiple)), l(lower));
    assert_eq!(l(input).rounded_up_to(l(multiple)), l(upper));
  }}
  rounded_to! {
    d01: (  0, 10,   0,   0,   0),
    d02: ( 10,  1,  10,  10,  10),
    d03: (-10,  1, -10, -10, -10),
    d04: (  8, 10,  10,   0,  10),
    d05: (  2, 10,   0,   0,  10),
    d06: ( -8, 10, -10, -10,   0),
    d07: ( -2, 10,   0, -10,   0),
    d08: ( 18, 10,  20,  10,  20),
    d09: ( 12, 10,  10,  10,  20),
    d10: (-18, 10, -20, -20, -10),
    d11: (-12, 10, -10, -20, -10),
    d12: ( 10, 10,  10,  10,  10),
    d13: (-10, 10, -10, -10, -10),
    d14: ( 20, 10,  20,  20,  20),
    d15: (-20, 10, -20, -20, -20),
    d16: ( 15, 10,  20,  10,  20),
    d17: (-15, 10, -20, -20, -10),
  }
}
