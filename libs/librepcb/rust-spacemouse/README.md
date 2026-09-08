# LibrePCB Rust SpaceMouse

*Claude AI assisted in the writing of this file.*
*It was reviewed and edited by a human.*

Provides a cross-platform (Windows/macOS/Linux) raw HID capture backend for 
3Dconnexion SpaceMouse and compatible 3D-mouse devices implementing
`IF_SpaceMouseInputBackend` on the C++ side (see
`libs/librepcb/editor/spacemouse/`). HID reports are read directly via 
`hidapi` rather than depending on 3Dconnexion's proprietary driver 
(3DxWare).

This library is automatically compiled through CMake/Corrosion whenever
LibrePCB is compiled. Developers may find it more convenient to work directly
in this directory with Cargo, however.

## Why `hidapi`

This crate depends on the `hidapi` crate for cross-platform HID access
(`hidraw` on Linux, `IOHIDManager` on macOS, `hid.dll` on Windows). By
default, `hidapi` vendors a small amount of C source code (the underlying
`hidapi` C library) and compiles it as part of `cargo build`, via the `cc`
crate.  It isn't a "100% pure-Rust" implementation.

This is intentional and doesn't conflict with LibrePCB's ask that code
maintained by developers be written in Rust instead of C++. The vendored C 
is invisible, build-time plumbing, similar to how Qt and several of 
`rust-core`'s own dependencies are C/C++ under the hood.

Licensing is also not a concern: the C `hidapi` library is dual/triple
licensed (GPLv2-or-later / a 3-clause BSD variant / the original permissive
"HIDAPI" license, at the user's choice) and the `hidapi` Rust crate itself
is MIT - both are unambiguously compatible with LibrePCB's GPLv3, with no
linking exception needed.

## Features

* `fail-on-warnings`: Turn compiler warnings into errors (used on CI).
* `ffi`: Compile with the foreign functions interface as called from the
  LibrePCB C++ code. Note: `cargo test` must be run *without* this feature.

## Build

    cargo build --features=ffi

## Check

    cargo clippy --features=ffi

## Test

    cargo test

## Build Documentation

    cargo doc --no-deps --document-private-items --features=ffi --open
