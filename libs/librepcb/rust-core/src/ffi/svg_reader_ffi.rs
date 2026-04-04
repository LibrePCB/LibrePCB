//! FFI for TODO

use super::cpp_ffi::*;
use crate::Length;
use std::vec::Vec;
use svg::*;

struct Point {
  x: Length,
  y: Length,
}

struct Angle(i64);

struct Vertex {
  pos: Point,
  angle: Angle,
}

struct Path(Vec<Vertex>);
