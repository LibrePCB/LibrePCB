//! Basic type for 2D coordinates.

use crate::Length;
use std::ops::{Add, Sub};

/// Basic type for 2D coordinates.
///
/// Represents a 2D coordinate (X/Y) using two [Length] objects.
///
/// # Note
///
/// This type is work in progress and expanded as needed.
///
/// # Safety
///
/// Any overflow through arithmetic operations on this type is considered
/// undefined behavior and may panic. Make sure you never create [Point]
/// objects with excessively large X/Y values. See details in [Length].
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct Point {
  /// X-coordinate
  pub x: Length,
  /// Y-coordinate
  pub y: Length,
}

impl Point {
  /// Create point (0, 0).
  pub fn zero() -> Self {
    Self {
      x: Length::zero(),
      y: Length::zero(),
    }
  }

  /// Create point from X/Y coordinates.
  pub fn new(x: Length, y: Length) -> Self {
    Self { x, y }
  }
}

impl Add for Point {
  type Output = Self;

  fn add(self, other: Self) -> Self {
    Self {
      x: self.x + other.x,
      y: self.y + other.y,
    }
  }
}

impl Sub for Point {
  type Output = Self;

  fn sub(self, other: Self) -> Self {
    Self {
      x: self.x - other.x,
      y: self.y - other.y,
    }
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  fn p(x_nm: i64, y_nm: i64) -> Point {
    Point::new(Length::from_nm(x_nm), Length::from_nm(y_nm))
  }

  #[test]
  fn assign() {
    #[allow(unused_assignments)]
    let mut a = p(10, 20);
    let mut b = p(30, 40);
    a = b;
    b = p(50, 60);
    assert_eq!(a, p(30, 40));
    assert_eq!(b, p(50, 60));
  }

  #[test]
  fn compare() {
    assert_eq!(p(1, 2) == p(1, 2), true);
    assert_eq!(p(1, 2) == p(2, 2), false);
    assert_eq!(p(1, 2) != p(2, 2), true);
    assert_eq!(p(1, 2) != p(1, 2), false);
  }

  #[test]
  fn arithmetic() {
    assert_eq!(p(-5, -10) + p(10, 20), p(5, 10));
    assert_eq!(p(5, 10) - p(10, 20), p(-5, -10));
  }

  #[test]
  fn zero() {
    assert_eq!(Point::zero().x.to_nm(), 0);
    assert_eq!(Point::zero().y.to_nm(), 0);
  }
}
