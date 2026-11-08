//! FFI for [`spacemouse::SpaceMouseLoop`].
//!

use spacemouse::{SpaceMouseEvent, SpaceMouseLoop};
use std::cell::Cell;
use std::os::raw::c_void;

/// Plain-old-data state of all six axes with a stable `#[repr(C)]` layout
/// for use across the FFI boundary.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct SpaceMouseMotionFfi {
  /// Translate left(-)/right(+)
  pub translation_x: i16,
  /// Translate away(-)/towards(+) the user
  pub translation_y: i16,
  /// Translate down(-)/up(+)
  pub translation_z: i16,
  /// Tilt (pitch)
  pub rotation_x: i16,
  /// Tilt (roll)
  pub rotation_y: i16,
  /// Twist (yaw)
  pub rotation_z: i16,
}

/// C ABI callback invoked whenever the device reports new motion.
///
/// <div class="warning">
/// Fires on a background thread owned by the Rust side, *not* the thread
/// that called [`ffi_spacemouse_loop_new`].
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

/// Opaque handle exposed to C++, wrapping a [`SpaceMouseLoop`].
pub struct FfiSpaceMouseLoop {
  /// The wrapped loop.
  inner: SpaceMouseLoop,
}

/// Create a new loop and start capturing device input in the background.
///
/// `user_data` is passed back unmodified to every callback. It must stay
/// valid until [`ffi_spacemouse_loop_free`] returns. Callbacks must not
/// call back into Rust synchronously.
///
/// Never returns null. Finding no device is not an error, it is reported
/// through `on_connected_changed`.
#[no_mangle]
extern "C" fn ffi_spacemouse_loop_new(
  user_data: *mut c_void,
  on_motion: MotionCallback,
  on_connected_changed: ConnectedCallback,
) -> *mut FfiSpaceMouseLoop {
  let ptr = SendPtr(user_data);
  // The C++ side expects the state of all six axes with every motion, but
  // the crate reports translation and rotation separately.
  let motion = Cell::new(SpaceMouseMotionFfi::default());
  let inner = SpaceMouseLoop::new(move |event| match event {
    SpaceMouseEvent::Connected => {
      motion.set(SpaceMouseMotionFfi::default());
      on_connected_changed(ptr.get(), true);
    }
    SpaceMouseEvent::Disconnected => {
      motion.set(SpaceMouseMotionFfi::default());
      on_connected_changed(ptr.get(), false);
    }
    SpaceMouseEvent::Translation { x, y, z } => {
      let mut m = motion.get();
      m.translation_x = x;
      m.translation_y = y;
      m.translation_z = z;
      motion.set(m);
      on_motion(ptr.get(), m);
    }
    SpaceMouseEvent::Rotation { x, y, z } => {
      let mut m = motion.get();
      m.rotation_x = x;
      m.rotation_y = y;
      m.rotation_z = z;
      motion.set(m);
      on_motion(ptr.get(), m);
    }
    _ => {}
  });
  Box::into_raw(Box::new(FfiSpaceMouseLoop { inner }))
}

/// Whether a compatible device is currently detected as connected.
#[no_mangle]
extern "C" fn ffi_spacemouse_loop_is_connected(
  handle: &FfiSpaceMouseLoop,
) -> bool {
  handle.inner.is_connected()
}

/// Set whether the device's LED (if it has one) should be lit.
///
/// Applied immediately if a device is open, and again on every connect.
/// Devices without an LED and write failures are ignored. See
/// [`SpaceMouseLoop::set_led`] for details.
#[no_mangle]
extern "C" fn ffi_spacemouse_loop_set_led(
  handle: &FfiSpaceMouseLoop,
  enabled: bool,
) {
  handle.inner.set_led(enabled);
}

/// Stop capturing, join the background thread, and free the loop.
///
/// # Safety (from the C++ side)
/// `handle` must be a non-null pointer previously returned by
/// [`ffi_spacemouse_loop_new`] and not already freed.
#[no_mangle]
extern "C" fn ffi_spacemouse_loop_free(handle: *mut FfiSpaceMouseLoop) {
  assert!(!handle.is_null());
  unsafe { drop(Box::from_raw(handle)) };
}
