// Claude AI assisted in the writing of this file.
// It was reviewed, and comments rewritten, by a human.

//! Raw HID input report parsing.
//!
//! Isolated from [`crate::hid`] specifically so it can be unit-tested
//! against captured byte sequences without any real hardware attached (see
//! the tests below).
//!
//! 3Dconnexion's wired-HID devices (as opposed to the newer "Wireless"/"Pro"
//! models that speak a different, undocumented report format) report
//! translation and rotation as two separate input reports on the same HID
//! interface:
//!
//! * Report ID `1`: 3 little-endian `i16` values - translation in X, Y, Z.
//! * Report ID `2`: 3 little-endian `i16` values - rotation about X, Y, Z.
//!
//! This matches the format used by `spacenavd` and other existing
//! open-source 3Dconnexion drivers, and has been confirmed correct against
//! a real 3Dconnexion SpaceMouse (translation, rotation, and axis signs
//! all behaving as expected in LibrePCB's schematic, PCB, and 3D views).

/// One decoded HID input report: either a translation or a rotation sample.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ReportAxes {
  /// Translation X/Y/Z, in that order.
  Translation(i16, i16, i16),
  /// Rotation X/Y/Z, in that order.
  Rotation(i16, i16, i16),
}

/// Report ID identifying a translation report.
const REPORT_ID_TRANSLATION: u8 = 1;
/// Report ID identifying a rotation report.
const REPORT_ID_ROTATION: u8 = 2;
/// Minimum report length (1 report-ID byte + 3 `i16` axis values) required
/// to decode a translation or rotation report.
const MIN_AXIS_REPORT_LEN: usize = 7;

/// Parse a raw HID input report, as returned by `hidapi`'s `read()`/
/// `read_timeout()` (report ID is the first byte, matching how `hidapi`
/// represents reports on every supported platform).
///
/// Returns `None` for reports this crate doesn't recognize (e.g. button
/// reports) or that are too short to contain the axes they claim to. It
/// never panics on malformed/truncated input, since this data ultimately
/// comes from external hardware.
pub fn parse_report(data: &[u8]) -> Option<ReportAxes> {
  let (&report_id, rest) = data.split_first()?;
  match report_id {
    REPORT_ID_TRANSLATION if data.len() >= MIN_AXIS_REPORT_LEN => {
      let (x, y, z) = read_three_axes(rest);
      Some(ReportAxes::Translation(x, y, z))
    }
    REPORT_ID_ROTATION if data.len() >= MIN_AXIS_REPORT_LEN => {
      let (x, y, z) = read_three_axes(rest);
      Some(ReportAxes::Rotation(x, y, z))
    }
    _ => None,
  }
}

/// Read three consecutive little-endian `i16` values from the start of
/// `data`. Caller must ensure `data` has at least 6 bytes.
fn read_three_axes(data: &[u8]) -> (i16, i16, i16) {
  (
    le_i16(&data[0..2]),
    le_i16(&data[2..4]),
    le_i16(&data[4..6]),
  )
}

/// Decode a 2-byte little-endian slice as `i16`. Caller must ensure `bytes`
/// has exactly 2 elements.
fn le_i16(bytes: &[u8]) -> i16 {
  i16::from_le_bytes([bytes[0], bytes[1]])
}

#[cfg(test)]
mod tests {
  use super::*;

  #[test]
  fn parses_translation_report() {
    // report ID 1, x=0x0010, y=0x0020, z=-16 (0xfff0)
    let data = [1u8, 0x10, 0x00, 0x20, 0x00, 0xf0, 0xff];
    assert_eq!(
      parse_report(&data),
      Some(ReportAxes::Translation(0x10, 0x20, -16))
    );
  }

  #[test]
  fn parses_rotation_report() {
    // report ID 2, x=-1 (0xffff), y=0, z=0x7fff
    let data = [2u8, 0xff, 0xff, 0x00, 0x00, 0xff, 0x7f];
    assert_eq!(
      parse_report(&data),
      Some(ReportAxes::Rotation(-1, 0, 0x7fff))
    );
  }

  #[test]
  fn rejects_unknown_report_id() {
    assert_eq!(parse_report(&[9, 0, 0, 0, 0, 0, 0]), None);
  }

  #[test]
  fn rejects_too_short_axis_report() {
    // Report ID 1, but missing the Z axis bytes.
    assert_eq!(parse_report(&[1, 0, 0, 0, 0]), None);
  }

  #[test]
  fn rejects_empty_report() {
    assert_eq!(parse_report(&[]), None);
  }
}
