//! Vertex type ([Point] + [Angle]).

use crate::Angle;
use crate::Point;

/// Basic vertex type ([Point] + [Angle]).
///
/// Represents a polygon/polyline vertex using a [Point] and an [Angle].
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct Vertex {
  /// Position
  pub pos: Point,
  /// Angle
  pub angle: Angle,
}

impl Vertex {
  /// Create a vertex with pos=(0, 0) and angle=0.
  pub fn zero() -> Self {
    Self {
      pos: Point::zero(),
      angle: Angle::zero(),
    }
  }

  /// Create a vertex from point only (0 degrees angle).
  pub fn new(pos: Point) -> Self {
    Self {
      pos,
      angle: Angle::zero(),
    }
  }

  /// Create a verzex from point & angle.
  pub fn new_with_angle(pos: Point, angle: Angle) -> Self {
    Self { pos, angle }
  }
}

#[cfg(test)]
mod tests {
  use super::*;
  use crate::Length;

  fn v(x_nm: i64, y_nm: i64, udeg: i64) -> Vertex {
    Vertex::new_with_angle(
      Point::new(Length::from_nm(x_nm), Length::from_nm(y_nm)),
      Angle::from_udeg(udeg),
    )
  }

  #[test]
  fn assign() {
    #[allow(unused_assignments)]
    let mut a = v(10, 20, 30);
    let mut b = v(30, 40, 50);
    a = b;
    b = v(50, 60, 70);
    assert_eq!(a, v(30, 40, 50));
    assert_eq!(b, v(50, 60, 70));
  }

  #[test]
  fn compare() {
    assert_eq!(v(1, 2, 3) == v(1, 2, 3), true);
    assert_eq!(v(1, 2, 3) == v(1, 2, 4), false);
    assert_eq!(v(1, 2, 3) != v(2, 2, 3), true);
    assert_eq!(v(1, 2, 3) != v(1, 2, 3), false);
  }

  #[test]
  fn zero() {
    let v = Vertex::zero();
    assert_eq!(v.pos.x.to_nm(), 0);
    assert_eq!(v.pos.y.to_nm(), 0);
    assert_eq!(v.angle.to_udeg(), 0);
  }

  #[test]
  fn new() {
    let v = Vertex::new(Point::new(Length::from_nm(1), Length::from_nm(2)));
    assert_eq!(v.pos.x.to_nm(), 1);
    assert_eq!(v.pos.y.to_nm(), 2);
    assert_eq!(v.angle.to_udeg(), 0);
  }
}
