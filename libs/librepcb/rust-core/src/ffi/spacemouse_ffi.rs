//! FFI for [`spacemouse::SpaceMouseBackend`].
//!

use spacemouse::{SpaceMouseBackend, SpaceMouseMotion};
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
  /// Tilt (roll)
  pub rotation_y: i16,
  /// Twist (yaw)
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
/// that called [`ffi_spacemouse_backend_new`].
/// </div>
pub type MotionCallback =
  extern "C" fn(user_data: *mut c_void, motion: SpaceMouseMotionFfi);

/// C ABI callback invoked whenever a compatible device is connected or
/// disconnected. Same threading caveat as [`MotionCallback`].
pub type ConnectedCallback =
  extern "C" fn(user_data: *mut c_void, connected: bool);

/// Wraps the user data pointer so it can be moved into the background thread.
/// Rust never dereferences it.
#[derive(Clone, Copy)]
struct SendPtr(*mut c_void);
unsafe impl Send for SendPtr {}

impl SendPtr {
  /// Get the wrapped pointer.
  ///
  /// Closures must use this instead of `.0` to capture the whole wrapper,
  /// and thus its `Send` impl, rather than the bare pointer field.
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
/// `user_data` is passed back unmodified to every callback. It must stay
/// valid until [`ffi_spacemouse_backend_free`] returns. Callbacks must not
/// call back into Rust synchronously.
///
/// Never returns null. Finding no device is not an error, it is reported
/// through `on_connected_changed`.
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
/// Applied immediately if a device is open, and again on every connect.
/// Devices without an LED and write failures are ignored. See
/// [`SpaceMouseBackend::set_led`] for details.
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
