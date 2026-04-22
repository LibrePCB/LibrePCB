# LibrePCB Rust Core

This library is automatically compiled through CMake/Corrosion whenever
LibrePCB is compiled. However, for development on this Rust library, it
is more convenient to work directly with Cargo in this directory.

## Features

* `fail-on-warnings`: Turn compiler warnings into errors (used on CI).
* `ffi`: Compile with the foreigh functions interface as called from the
  LibrePCB C++ code. Note: `cargo test` must be run *without* this feature.

## Build

    cargo build --features=ffi

## Check

    cargo clippy --features=ffi

## Test

    cargo test

## Measure Test Coverage

    cargo install cargo-llvm-cov
    cargo llvm-cov --show-missing-lines --html --open

See also https://rustprojectprimer.com/measure/coverage.html.

## Build Documentation

    cargo doc --no-deps --document-private-items --features=ffi --open
