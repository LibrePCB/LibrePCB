// Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

//! Device identification and the raw motion data type.

/// USB vendor ID used by current 3Dconnexion devices.
pub const VENDOR_ID_3DCONNEXION: u16 = 0x256f;

/// Legacy USB vendor ID used by early (Logitech-era) 3Dconnexion devices.
pub const VENDOR_ID_LOGITECH_3DCONNEXION: u16 = 0x046d;

/// All USB vendor IDs the device must match to be considered, in no
/// particular priority order.
pub const KNOWN_VENDOR_IDS: [u16; 2] =
  [VENDOR_ID_3DCONNEXION, VENDOR_ID_LOGITECH_3DCONNEXION];

/// HID usage page identifying "Generic Desktop Controls".
pub const USAGE_PAGE_GENERIC_DESKTOP: u16 = 0x01;

/// HID usage ID identifying a "Multi-axis Controller" within the Generic
/// Desktop Controls usage page - the usage LibrePCB filters on, matching
/// what the previous Win32 Raw Input backend already did.
pub const USAGE_ID_MULTI_AXIS_CONTROLLER: u16 = 0x08;

/// Raw, unscaled motion values reported by a 3D mouse.
///
/// Mirrors C++'s `SpaceMouseMotionEvent`
/// (`libs/librepcb/editor/spacemouse/if_spacemouseinputbackend.h`) field for
/// field. Values are whatever the device's translation/rotation HID reports
/// contain - no sensitivity scaling, dead-zone handling, or calibration is
/// applied here (see the crate-level docs).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct SpaceMouseMotion {
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
