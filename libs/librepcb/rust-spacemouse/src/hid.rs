//! `hidapi`-based background capture thread.

// AI DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

use crate::device::{
  SpaceMouseMotion, KNOWN_VENDOR_IDS, USAGE_ID_MULTI_AXIS_CONTROLLER,
  USAGE_PAGE_GENERIC_DESKTOP,
};
use crate::report::{parse_report, ReportAxes};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc};
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
/// without a compatible device connected: The background thread simply
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
  /// Sends desired LED on/off state into the background thread - see
  /// [`SpaceMouseBackend::set_led`].
  led_sender: mpsc::Sender<bool>,
  /// The background thread, joined on drop. Always `Some` until
  /// [`Drop::drop`] runs.
  thread: Option<thread::JoinHandle<()>>,
}

impl SpaceMouseBackend {
  /// Start capturing in a new background thread.
  ///
  /// `on_motion` is invoked (on the background thread, *not* the caller's
  /// thread) with the full, merged six-axis state every time a report is 
  /// decoded. `on_connected_changed` is invoked (also on the background 
  /// thread) whenever a compatible device is opened or lost. Callers that
  /// need these on a specific thread (e.g. a Qt object's own thread) are
  /// responsible for the hop themselves; see `spacemouseinputrust.cpp` for
  /// how the C++ side does this via `QMetaObject::invokeMethod`.
  pub fn new<M, C>(on_motion: M, on_connected_changed: C) -> Self
  where
    M: Fn(SpaceMouseMotion) + Send + 'static,
    C: Fn(bool) + Send + 'static,
  {
    let stop = Arc::new(AtomicBool::new(false));
    let connected = Arc::new(AtomicBool::new(false));
    let (led_sender, led_receiver) = mpsc::channel();
    let thread_stop = Arc::clone(&stop);
    let thread_connected = Arc::clone(&connected);
    let thread = thread::spawn(move || {
      capture_loop(
        &thread_stop,
        &thread_connected,
        &led_receiver,
        &on_motion,
        &on_connected_changed,
      );
    });
    Self {
      stop,
      connected,
      led_sender,
      thread: Some(thread),
    }
  }

  /// Whether a compatible device is currently open.
  pub fn is_connected(&self) -> bool {
    self.connected.load(Ordering::Relaxed)
  }

  /// Set whether the device's LED should be lit.
  ///
  /// Applied immediately if a device is currently open, and reapplied
  /// automatically the next time one is (re)opened (see [`capture_loop`]),
  /// so a disconnect/reconnect doesn't need this called again. Errors
  /// (i.e., the device doesn't have an LED, or there's a transient write 
  /// failure) are silently ignored.
  ///
  /// The underlying channel send can only fail if the background thread
  /// has already exited (e.g. concurrently with [`Drop::drop`]), which is
  /// equally harmless to ignore here.
  pub fn set_led(&self, enabled: bool) {
    let _ = self.led_sender.send(enabled);
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
  led_receiver: &mpsc::Receiver<bool>,
  on_motion: &M,
  on_connected_changed: &C,
) where
  M: Fn(SpaceMouseMotion),
  C: Fn(bool),
{
  // The last LED state requested via `SpaceMouseBackend::set_led()`, is
  // applied upon device (re)open, not just when the command arrives.  This
  // ensures "enable LED on connection" holds across unplug/replug cycles. 
  // The LED defaults to 'on'.  Note that the C++ side normally sends an 
  // explicit command right after construction reflecting the user preference,
  // so the default is only in effect for a short period of time.
  let mut desired_led = true;

  while !stop.load(Ordering::Relaxed) {
    // Track any LED command(s) that arrive even if no device is connected.
    // This ensures that when a device is connected, its LED is set to the
    // correct state.
    while let Ok(enabled) = led_receiver.try_recv() {
      desired_led = enabled;
    }

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
    write_led(&device, desired_led);

    let mut motion = SpaceMouseMotion::default();
    let mut buf = [0u8; READ_BUF_LEN];
    loop {
      if stop.load(Ordering::Relaxed) {
        connected.store(false, Ordering::Relaxed);
        on_connected_changed(false);
        return;
      }
      let mut led_changed = false;
      while let Ok(enabled) = led_receiver.try_recv() {
        if enabled != desired_led {
          desired_led = enabled;
          led_changed = true;
        }
      }
      if led_changed {
        write_led(&device, desired_led);
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

/// Write the HID output report that controls the device's LED.
///
/// All LED-capable devices across the SpaceNavigator/SpaceMouse/SpacePilot 
/// product line utilize the same HID report format:
/// Report ID `0x04` + a single data byte (`0x01` on / `0x00` off)
/// This format has been confirmed against PySpaceMouse's own raw-`hidapi` 
/// `set_led()` and its per-model device table. Best-effort: not every device 
/// has an LED, and there's no reliable way to detect a transient write 
/// failure, so both are silently ignored here.
fn write_led(device: &hidapi::HidDevice, enabled: bool) {
  let _ = device.write(&[0x04, if enabled { 0x01 } else { 0x00 }]);
}

/// Find the first currently-connected, compatible 3D-mouse HID interface.
fn find_device(api: &hidapi::HidApi) -> Option<&hidapi::DeviceInfo> {
  api.device_list().find(|d| {
    KNOWN_VENDOR_IDS.contains(&d.vendor_id())
      && d.usage_page() == USAGE_PAGE_GENERIC_DESKTOP
      && d.usage() == USAGE_ID_MULTI_AXIS_CONTROLLER
  })
}
