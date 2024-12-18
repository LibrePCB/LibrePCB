extern crate cbindgen;

use std::env;

fn main() {
  let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

  if let Ok(r) = cbindgen::generate(crate_dir) {
    r.write_to_file("ffi.h");
  } else {
    // Do not fail the build on invalid source files as this is annoying
    // during development beause it would suppress the compiler errors.
    println!("cargo::warning=Failed to generate ffi.h with cbindgen.");
  }
}
