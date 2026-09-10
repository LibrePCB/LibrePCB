//! FFI for [`crate::hid::SpaceMouseBackend`].
//!
//! Deliberately tiny: an opaque handle, a constructor taking two C-ABI
//! callbacks, an `is_connected()` query, and a destructor. Everything else
//! (motion mapping, sensitivity, calibration, dispatch) is a C++-side
//! concern.  See the crate-level docs for more information.

// AI DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

use crate::device::SpaceMouseMotion;
use crate::hid::SpaceMouseBackend;
use std::os::raw::c_void;

/// Plain-old-data mirror of [`SpaceMouseMotion`] with a stable `#[repr(C)]`
/// layout for use across the FFI boundary.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct SpaceMouseMotionFfi {
  /// Pan left(-)/right(+)
  pub translation_x: i16,
  /// Pan away(-)/towards(+) the user
  pub translation_y: i16,
  /// Pan/zoom down(-)/up(+)
  pub translation_z: i16,
  /// Tilt (pitch)
  pub rotation_x: i16,
  /// Tilt (yaw)
  pub rotation_y: i16,
  /// Twist (roll)
  pub rotation_z: i16,
}

impl From<SpaceMouseMotion> for SpaceMouseMotionFfi {
  fn from(m: SpaceMouseMotion) -> Self {
    Self {
      translation_x: m.translation_x,
      translation_y: m.translation_y,
      translation_z: m.translation_z,
      rotation_x: m.rotation_x,
      rotation_y: m.rotation_y,
      rotation_z: m.rotation_z,
    }
  }
}

/// C ABI callback invoked whenever the device reports new motion.
///
/// <div class="warning">
/// Fires on a background thread owned by the Rust side, *not* the thread
/// that called [`ffi_spacemouse_backend_new`]. The C++ side is responsible
/// for hopping onto whatever thread it needs before touching anything not
/// safe to call from an arbitrary thread (e.g. before emitting a Qt
/// signal).  Also see `spacemouseinputrust.cpp`.
/// </div>
pub type MotionCallback =
  extern "C" fn(user_data: *mut c_void, motion: SpaceMouseMotionFfi);

/// C ABI callback invoked whenever a compatible device is connected or
/// disconnected. Same threading caveat as [`MotionCallback`].
pub type ConnectedCallback =
  extern "C" fn(user_data: *mut c_void, connected: bool);

/// Wraps a raw `*mut c_void` so it can be moved into the background thread
/// spawned by [`SpaceMouseBackend::new`]. Safe because Rust never
/// dereferences it.  It's only ever handed back, unmodified, to the C++
/// callbacks that gave it to us.
#[derive(Clone, Copy)]
struct SendPtr(*mut c_void);
unsafe impl Send for SendPtr {}

impl SendPtr {
  /// Get the wrapped pointer back out.
  ///
  /// Deliberately a method rather than exposing the field directly: with
  /// Rust 2021's disjoint closure captures, a closure that writes `ptr.0`
  /// instead of `ptr.get()` would capture just that `*mut c_void` field on
  /// its own (bypassing the `SendPtr` wrapper's `unsafe impl Send`
  /// entirely) instead of the whole `Send`-able `SendPtr`, and fail to
  /// compile with a "`*mut c_void` cannot be sent between threads safely"
  /// error.
  fn get(self) -> *mut c_void {
    self.0
  }
}

/// Opaque handle exposed to C++, wrapping a [`SpaceMouseBackend`].
pub struct FfiSpaceMouseBackend {
  /// The wrapped backend.
  inner: SpaceMouseBackend,
}

/// Create a new backend and start capturing device input in the background.
///
/// `user_data` is passed back unmodified as the first argument of every
/// callback invocation; Rust never dereferences it. The caller must keep
/// whatever it points to alive until after [`ffi_spacemouse_backend_free`]
/// returns, and must not call back into Rust synchronously from within a 
/// callback (there is no re-entrancy protection).
///
/// Never returns null: Unlike opening a specific device, constructing the
/// backend itself cannot fail.  "No compatible device found (yet)" is not
/// an error, it's the normal state before `on_connected_changed(true)` is
/// ever invoked.
#[no_mangle]
extern "C" fn ffi_spacemouse_backend_new(
  user_data: *mut c_void,
  on_motion: MotionCallback,
  on_connected_changed: ConnectedCallback,
) -> *mut FfiSpaceMouseBackend {
  let ptr = SendPtr(user_data);
  let inner = SpaceMouseBackend::new(
    move |motion| on_motion(ptr.get(), motion.into()),
    move |connected| on_connected_changed(ptr.get(), connected),
  );
  Box::into_raw(Box::new(FfiSpaceMouseBackend { inner }))
}

/// Whether a compatible device is currently detected as connected.
#[no_mangle]
extern "C" fn ffi_spacemouse_backend_is_connected(
  backend: &FfiSpaceMouseBackend,
) -> bool {
  backend.inner.is_connected()
}

/// Set whether the device's LED (if it has one) should be lit.
///
/// A one-shot command: applied immediately if a device is currently open,
/// and (re-)applied automatically any time the device connects.  Devices
/// without an LED, and any transient write failure, are both silently 
/// ignored.  See [`SpaceMouseBackend::set_led`] for additional info.
#[no_mangle]
extern "C" fn ffi_spacemouse_backend_set_led(
  backend: &FfiSpaceMouseBackend,
  enabled: bool,
) {
  backend.inner.set_led(enabled);
}

/// Stop capturing, join the background thread, and free the backend.
///
/// # Safety (from the C++ side)
/// `backend` must be a non-null pointer previously returned by
/// [`ffi_spacemouse_backend_new`] and not already freed.
#[no_mangle]
extern "C" fn ffi_spacemouse_backend_free(backend: *mut FfiSpaceMouseBackend) {
  assert!(!backend.is_null());
  unsafe { drop(Box::from_raw(backend)) };
}
