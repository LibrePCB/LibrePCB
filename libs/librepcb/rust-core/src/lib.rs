//! LibrePCB Core Library

// Fail on warnings if feature "fail-on-warnings" is enabled.
#![cfg_attr(feature = "fail-on-warnings", deny(warnings))]
#![warn(missing_docs)]
#![warn(clippy::missing_docs_in_private_items)]

// Modules.
mod ffi;

// Allow using the create! macro to create parametrized tests in test modules
// without explicitly importing the parameterized_test crate every time.
#[cfg(test)]
#[macro_use]
extern crate parameterized_test;
