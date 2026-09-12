// Claude AI assisted in modifications to this file.
// It was reviewed by a human.

//! FFI for Rust modules, to be used from C++ through cbindgen.

mod angle_ffi;
mod cpp_ffi;
mod ibom_ffi;
mod length_ffi;
mod math_ffi;
#[cfg(feature = "spacemouse-hid")]
mod spacemouse_ffi;
mod toolbox_ffi;
mod zip_archive_ffi;
mod zip_writer_ffi;
