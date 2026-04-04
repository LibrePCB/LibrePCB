//! Vec<[Vertex]> type (called `Path` in C++)

use crate::Vertex;
use std::ops::{Deref, DerefMut};

/// Vec<[Vertex]> type (called `Path` in C++)
///
/// Represents a vector of [Vertex] objects. This type is called `Path` in
/// C++.
///
/// # Note
///
/// This type is work in progress and expanded as needed.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct VertexVec(pub Vec<Vertex>);

impl Deref for VertexVec {
  type Target = Vec<Vertex>;

  fn deref(&self) -> &Self::Target {
    &self.0
  }
}

impl DerefMut for VertexVec {
  fn deref_mut(&mut self) -> &mut Self::Target {
    &mut self.0
  }
}

impl IntoIterator for VertexVec {
  type Item = Vertex;
  type IntoIter = std::vec::IntoIter<Vertex>;

  fn into_iter(self) -> Self::IntoIter {
    self.0.into_iter()
  }
}

impl<'a> IntoIterator for &'a VertexVec {
  type Item = &'a Vertex;
  type IntoIter = std::slice::Iter<'a, Vertex>;

  fn into_iter(self) -> Self::IntoIter {
    self.0.iter()
  }
}

impl<'a> IntoIterator for &'a mut VertexVec {
  type Item = &'a mut Vertex;
  type IntoIter = std::slice::IterMut<'a, Vertex>;

  fn into_iter(self) -> Self::IntoIter {
    self.0.iter_mut()
  }
}
