// Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

//! `hidapi`-based background capture thread.

use crate::device::{
  SpaceMouseMotion, KNOWN_VENDOR_IDS, USAGE_ID_MULTI_AXIS_CONTROLLER,
  USAGE_PAGE_GENERIC_DESKTOP,
};
use crate::report::{parse_report, ReportAxes};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

/// How long to wait between (re-)scans while no compatible device is open,
/// to avoid busy-looping when nothing is connected or a `HidApi`/open call
/// keeps failing.
const RESCAN_INTERVAL: Duration = Duration::from_millis(500);

/// `hidapi` blocking-read timeout while a device is open. Chosen short
/// enough that a `stop()` request is noticed promptly, long enough to avoid
/// busy-looping the background thread when the device is idle.
const READ_TIMEOUT_MS: i32 = 200;

/// HID input reports from these devices are at most a handful of bytes;
/// this is generous headroom.
const READ_BUF_LEN: usize = 64;

/// Cross-platform SpaceMouse HID capture backend.
///
/// Owns a background thread that continuously (re-)discovers a compatible
/// device, reads its raw HID reports, and forwards decoded motion /
/// connection-state changes to the callbacks given to
/// [`SpaceMouseBackend::new`]. Constructing one is a safe no-op on a machine
/// without a compatible device connected - the background thread simply
/// keeps quietly re-scanning (see [`RESCAN_INTERVAL`]) until one shows up,
/// and keeps doing so again after a device is unplugged, so hot-plugging
/// "just works" without recreating this object.
pub struct SpaceMouseBackend {
  /// Signals the background thread to stop and return.
  stop: Arc<AtomicBool>,
  /// Whether a compatible device is currently open. Kept outside the
  /// thread so [`SpaceMouseBackend::is_connected`] doesn't have to
  /// synchronize with it via the callbacks.
  connected: Arc<AtomicBool>,
  /// The background thread, joined on drop. Always `Some` until
  /// [`Drop::drop`] runs.
  thread: Option<thread::JoinHandle<()>>,
}

impl SpaceMouseBackend {
  /// Start capturing in a new background thread.
  ///
  /// `on_motion` is invoked (on the background thread, *not* the caller's
  /// thread) with the full, merged six-axis state every time a translation
  /// or rotation report is decoded. `on_connected_changed` is invoked (also 
  /// on the background thread) whenever a compatible device is opened or 
  /// lost. Callers that need these on a specific thread (e.g. a Qt object's 
  /// own thread) are responsible for the hop themselves; see 
  /// `spacemouseinputrust.cpp` for how the C++ side does this via 
  /// `QMetaObject::invokeMethod`.
  pub fn new<M, C>(on_motion: M, on_connected_changed: C) -> Self
  where
    M: Fn(SpaceMouseMotion) + Send + 'static,
    C: Fn(bool) + Send + 'static,
  {
    let stop = Arc::new(AtomicBool::new(false));
    let connected = Arc::new(AtomicBool::new(false));
    let thread_stop = Arc::clone(&stop);
    let thread_connected = Arc::clone(&connected);
    let thread = thread::spawn(move || {
      capture_loop(
        &thread_stop,
        &thread_connected,
        &on_motion,
        &on_connected_changed,
      );
    });
    Self {
      stop,
      connected,
      thread: Some(thread),
    }
  }

  /// Whether a compatible device is currently open.
  pub fn is_connected(&self) -> bool {
    self.connected.load(Ordering::Relaxed)
  }
}

impl Drop for SpaceMouseBackend {
  fn drop(&mut self) {
    self.stop.store(true, Ordering::Relaxed);
    if let Some(t) = self.thread.take() {
      // Only fails if the background thread itself panicked; there is no
      // sensible way to surface that from a `Drop` impl, so it's dropped
      // here rather than propagated.
      let _ = t.join();
    }
  }
}

/// Body of the background capture thread.
///
/// Repeatedly looks for a compatible device, reads from it until it's lost
/// (or `stop` is set), then goes back to looking for one - so a
/// hot-unplugged/replugged device recovers on its own without recreating
/// [`SpaceMouseBackend`].
fn capture_loop<M, C>(
  stop: &AtomicBool,
  connected: &AtomicBool,
  on_motion: &M,
  on_connected_changed: &C,
) where
  M: Fn(SpaceMouseMotion),
  C: Fn(bool),
{
  while !stop.load(Ordering::Relaxed) {
    let api = match hidapi::HidApi::new() {
      Ok(api) => api,
      Err(_) => {
        thread::sleep(RESCAN_INTERVAL);
        continue;
      }
    };

    let Some(info) = find_device(&api) else {
      thread::sleep(RESCAN_INTERVAL);
      continue;
    };

    let device = match info.open_device(&api) {
      Ok(d) => d,
      Err(_) => {
        thread::sleep(RESCAN_INTERVAL);
        continue;
      }
    };

    connected.store(true, Ordering::Relaxed);
    on_connected_changed(true);

    let mut motion = SpaceMouseMotion::default();
    let mut buf = [0u8; READ_BUF_LEN];
    loop {
      if stop.load(Ordering::Relaxed) {
        connected.store(false, Ordering::Relaxed);
        on_connected_changed(false);
        return;
      }
      match device.read_timeout(&mut buf, READ_TIMEOUT_MS) {
        // Timed out without any data - just loop around to re-check `stop`.
        Ok(0) => continue,
        Ok(len) => {
          if let Some(axes) = parse_report(&buf[..len]) {
            match axes {
              ReportAxes::Translation(x, y, z) => {
                motion.translation_x = x;
                motion.translation_y = y;
                motion.translation_z = z;
              }
              ReportAxes::Rotation(x, y, z) => {
                motion.rotation_x = x;
                motion.rotation_y = y;
                motion.rotation_z = z;
              }
            }
            on_motion(motion);
          }
        }
        // The device is gone (unplugged, I/O error, ...): stop reading and 
        // go back to scanning for a (possibly different) device.
        Err(_) => break,
      }
    }

    connected.store(false, Ordering::Relaxed);
    on_connected_changed(false);
  }
}

/// Find the first currently-connected, compatible 3D-mouse HID interface.
fn find_device(api: &hidapi::HidApi) -> Option<&hidapi::DeviceInfo> {
  api.device_list().find(|d| {
    KNOWN_VENDOR_IDS.contains(&d.vendor_id())
      && d.usage_page() == USAGE_PAGE_GENERIC_DESKTOP
      && d.usage() == USAGE_ID_MULTI_AXIS_CONTROLLER
  })
}
