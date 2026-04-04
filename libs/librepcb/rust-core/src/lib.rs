//! LibrePCB Core Library

// Fail on warnings if feature "fail-on-warnings" is enabled.
#![cfg_attr(feature = "fail-on-warnings", deny(warnings))]
#![warn(missing_docs)]
#![warn(clippy::missing_docs_in_private_items)]

// Build FFI only if explicitly enabled. This allows to use the crate also
// outside of LibrePCB (i.e. without C++ integration), and fixes unresolved
// symbol linker errors when building the tests.
#[cfg(feature = "ffi")]
mod ffi;

// Modules
pub mod math;
pub mod svg_reader;
pub mod toolbox;
mod types;

// Re-Exports
pub use types::angle::Angle;
pub use types::length::Length;
pub use types::point::Point;
pub use types::vertex::Vertex;
pub use types::vertex_vec::VertexVec;

// Allow using the create! macro to create parametrized tests in test modules
// without explicitly importing the parameterized_test crate every time.
#[cfg(test)]
#[macro_use]
extern crate parameterized_test;
