// Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

//! LibrePCB Rust SpaceMouse
//!
//! Cross-platform (Windows/macOS/Linux) raw HID capture for 3Dconnexion
//! SpaceMouse (and compatible) 3D-mouse devices. See `README.md` for
//! more information.
//!
//! This crate deliberately knows nothing about sensitivity, inversion,
//! calibration or dead-zones. It always reports the device's raw,
//! unscaled axis readings. Data manipulation is a concern of the C++ side
//! so the FFI boundary stays tiny and "dumb".

#![cfg_attr(feature = "fail-on-warnings", deny(warnings))]
#![warn(missing_docs)]
#![warn(clippy::missing_docs_in_private_items)]

// Build FFI only if explicitly enabled. This allows using the crate 
// outside of LibrePCB (i.e. without C++ integration), and fixes unresolved
// symbol linker errors when building the tests.
#[cfg(feature = "ffi")]
mod ffi;

pub mod device;
mod hid;
mod report;

// Re-Exports
pub use device::SpaceMouseMotion;
pub use hid::SpaceMouseBackend;
