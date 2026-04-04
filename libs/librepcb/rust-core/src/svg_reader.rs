//! SVG file reader
use crate::{Angle, Length, Point, Vertex, VertexVec};
use std::f64::consts::PI;
use std::path::Path;

use svg::node::element::path::{Command, Data, Position};
use svg::node::element::tag;
use svg::parser::Event;
use svg::Parser;
use thiserror::Error;

// ── Error type ────────────────────────────────────────────────────────────────

#[derive(Debug, Error)]
pub enum SvgParseError {
  #[error("I/O error: {0}")]
  Io(#[from] std::io::Error),

  #[error("SVG path data could not be parsed: {0}")]
  PathData(String),

  #[error("EllipticalArc parameters must be a multiple of 7, got {0}")]
  ArcParamCount(usize),

  #[error("No current point when command '{0}' was encountered")]
  NoCurrentPoint(&'static str),
}

// ── Unit conversion helpers ───────────────────────────────────────────────────

/// SVG user units -> nanometres.
///
/// SVG user units are assumed to be millimetres here (1 mm = 1 000 000 nm).
/// Adjust `MM_PER_USER_UNIT` if your SVGs use a different convention.
const MM_PER_USER_UNIT: f64 = 1.0;
const NM_PER_MM: f64 = 1_000_000.0;

#[inline]
pub fn uu_to_nm(v: f64) -> i64 {
  (v * MM_PER_USER_UNIT * NM_PER_MM).round() as i64
}

#[inline]
fn rad_to_udeg(r: f64) -> Angle {
  Angle::from_udeg_f(r.to_degrees() * 1_000_000.0).unwrap()
}

#[inline]
fn pt(x: f64, y: f64) -> Point {
  Point::new(Length::from_nm(uu_to_nm(x)), Length::from_nm(uu_to_nm(y)))
}

// ── Arc tessellation ──────────────────────────────────────────────────────────

/// Convert SVG endpoint-arc parameters to centre-arc parameters and emit
/// vertices with correct tangent angles.
///
/// SVG arc: A rx ry x-rotation large-arc-flag sweep-flag x y
/// All parameters are for the *absolute* destination (caller resolves relative).
///
/// The tessellation angle step is `ARC_STEP_DEG` degrees; reduce for finer
/// curves.
const ARC_STEP_DEG: f64 = 1.0;

fn arc_vertices(
  x1: f64,
  y1: f64,
  rx: f64,
  ry: f64,
  phi_deg: f64,
  large_arc: bool,
  sweep: bool,
  x2: f64,
  y2: f64,
) -> VertexVec {
  // Degenerate: zero radii -> straight line to endpoint
  if rx == 0.0 || ry == 0.0 {
    let angle_rad = (y2 - y1).atan2(x2 - x1);
    return VertexVec(vec![Vertex::new_with_angle(
      pt(x2, y2),
      rad_to_udeg(angle_rad),
    )]);
  }

  let phi = phi_deg.to_radians();
  let (cos_phi, sin_phi) = (phi.cos(), phi.sin());

  // Step 1: x1' y1' (SVG spec F.6.5.1)
  let dx = (x1 - x2) / 2.0;
  let dy = (y1 - y2) / 2.0;
  let x1p = cos_phi * dx + sin_phi * dy;
  let y1p = -sin_phi * dx + cos_phi * dy;

  // Ensure radii are large enough (F.6.6.3)
  let mut rx = rx.abs();
  let mut ry = ry.abs();
  let lambda = (x1p / rx).powi(2) + (y1p / ry).powi(2);
  if lambda > 1.0 {
    let s = lambda.sqrt();
    rx *= s;
    ry *= s;
  }

  // Step 2: centre in primed coordinates (F.6.5.2)
  let num = (rx * ry).powi(2) - (rx * y1p).powi(2) - (ry * x1p).powi(2);
  let den = (rx * y1p).powi(2) + (ry * x1p).powi(2);
  let sq = if den == 0.0 {
    0.0
  } else {
    (num / den).abs().sqrt()
  };
  let sign = if large_arc == sweep { -1.0 } else { 1.0 };
  let cxp = sign * sq * rx * y1p / ry;
  let cyp = -sign * sq * ry * x1p / rx;

  // Step 3: centre in original coordinates (F.6.5.3)
  let cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2.0;
  let cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2.0;

  // Step 4: start/end angles (F.6.5.5 / F.6.5.6)
  let ux = (x1p - cxp) / rx;
  let uy = (y1p - cyp) / ry;
  let vx = (-x1p - cxp) / rx;
  let vy = (-y1p - cyp) / ry;

  let theta1 = angle_between(1.0, 0.0, ux, uy);
  let mut d_theta = angle_between(ux, uy, vx, vy);

  if !sweep && d_theta > 0.0 {
    d_theta -= 2.0 * PI;
  } else if sweep && d_theta < 0.0 {
    d_theta += 2.0 * PI;
  }

  // Tessellate
  let n_steps =
    ((d_theta.abs().to_degrees() / ARC_STEP_DEG).ceil() as usize).max(1);
  let mut verts = Vec::with_capacity(n_steps);

  for i in 1..=n_steps {
    let t = i as f64 / n_steps as f64;
    let theta = theta1 + t * d_theta;

    // Point on ellipse in original coordinates
    let ex = cos_phi * rx * theta.cos() - sin_phi * ry * theta.sin() + cx;
    let ey = sin_phi * rx * theta.cos() + cos_phi * ry * theta.sin() + cy;

    // Tangent direction (derivative w.r.t. theta), rotated by phi
    let dtx = -rx * theta.sin();
    let dty = ry * theta.cos();
    let tang_x = cos_phi * dtx - sin_phi * dty;
    let tang_y = sin_phi * dtx + cos_phi * dty;
    let tang_angle = if sweep {
      tang_y.atan2(tang_x)
    } else {
      (-tang_y).atan2(-tang_x)
    };

    verts.push(Vertex::new_with_angle(pt(ex, ey), rad_to_udeg(tang_angle)));
  }

  VertexVec(verts)
}

/// Signed angle between vectors (u, v) in radians.
fn angle_between(ux: f64, uy: f64, vx: f64, vy: f64) -> f64 {
  let dot = ux * vx + uy * vy;
  let len = ((ux * ux + uy * uy) * (vx * vx + vy * vy)).sqrt();
  let cos_a = (dot / len).clamp(-1.0, 1.0);
  let sign = if ux * vy - uy * vx < 0.0 { -1.0 } else { 1.0 };
  sign * cos_a.acos()
}

// ── Core parser ───────────────────────────────────────────────────────────────

/// Parse `<path>` command data into a list of `VertexVec` chains.
///
/// Each `Move` command starts a new chain. `Close` re-emits the first vertex
/// with the closing tangent and also starts a new chain.
fn parse_path_data(data: &Data) -> Result<Vec<VertexVec>, SvgParseError> {
  let mut all: Vec<VertexVec> = Vec::new();
  let mut current: VertexVec = VertexVec(Vec::new());

  // Tracks absolute current point and the start of the current sub-path.
  let mut cur_x = 0.0_f64;
  let mut cur_y = 0.0_f64;
  let mut start_x = 0.0_f64;
  let mut start_y = 0.0_f64;
  // Did we seed current[] with a placeholder start vertex?
  let mut has_start = false;

  for command in data.iter() {
    match command {
      // ── Move ──────────────────────────────────────────────────────────
      Command::Move(pos, params) => {
        if !current.is_empty() && has_start {
          all.push(VertexVec(std::mem::take(&mut current)));
        }
        has_start = false;

        let coords: Vec<f64> = params.iter().map(|&v| v as f64).collect();
        let mut first = true;
        for chunk in coords.chunks(2) {
          if chunk.len() < 2 {
            break;
          }
          let (x, y) = match pos {
            Position::Absolute => (chunk[0], chunk[1]),
            Position::Relative => (cur_x + chunk[0], cur_y + chunk[1]),
          };
          if first {
            cur_x = x;
            cur_y = y;
            start_x = x;
            start_y = y;
            // Placeholder start vertex; angle filled in by next segment.
            current.push(Vertex::new(pt(x, y)));
            has_start = true;
            first = false;
          } else {
            // Subsequent pairs are implicit LineTo (SVG §8.3.3).
            let angle_rad = (y - cur_y).atan2(x - cur_x);
            if let Some(last) = current.last_mut() {
              last.angle = rad_to_udeg(angle_rad);
            }
            current
              .push(Vertex::new_with_angle(pt(x, y), rad_to_udeg(angle_rad)));
            cur_x = x;
            cur_y = y;
          }
        }
      }

      // ── Line ──────────────────────────────────────────────────────────
      Command::Line(pos, params) => {
        let coords: Vec<f64> = params.iter().map(|&v| v as f64).collect();
        for chunk in coords.chunks(2) {
          if chunk.len() < 2 {
            break;
          }
          let (x2, y2) = match pos {
            Position::Absolute => (chunk[0], chunk[1]),
            Position::Relative => (cur_x + chunk[0], cur_y + chunk[1]),
          };
          let angle_rad = (y2 - cur_y).atan2(x2 - cur_x);
          // Back-fill the angle of the previous (start) vertex.
          if let Some(last) = current.last_mut() {
            last.angle = rad_to_udeg(angle_rad);
          }
          current
            .push(Vertex::new_with_angle(pt(x2, y2), rad_to_udeg(angle_rad)));
          cur_x = x2;
          cur_y = y2;
        }
      }

      // ── Horizontal line ───────────────────────────────────────────────
      Command::HorizontalLine(pos, params) => {
        for &p in params.iter() {
          let x2 = match pos {
            Position::Absolute => p as f64,
            Position::Relative => cur_x + p as f64,
          };
          let y2 = cur_y;
          let angle_rad = if x2 >= cur_x { 0.0 } else { PI };
          if let Some(last) = current.last_mut() {
            last.angle = rad_to_udeg(angle_rad);
          }
          current
            .push(Vertex::new_with_angle(pt(x2, y2), rad_to_udeg(angle_rad)));
          cur_x = x2;
        }
      }

      // ── Vertical line ─────────────────────────────────────────────────
      Command::VerticalLine(pos, params) => {
        for &p in params.iter() {
          let x2 = cur_x;
          let y2 = match pos {
            Position::Absolute => p as f64,
            Position::Relative => cur_y + p as f64,
          };
          // SVG y-axis points down; angle from +x axis
          let angle_rad = if y2 >= cur_y { PI / 2.0 } else { -PI / 2.0 };
          if let Some(last) = current.last_mut() {
            last.angle = rad_to_udeg(angle_rad);
          }
          current
            .push(Vertex::new_with_angle(pt(x2, y2), rad_to_udeg(angle_rad)));
          cur_y = y2;
        }
      }

      // ── Elliptical arc ─────────────────────────────────────────────────
      Command::EllipticalArc(pos, params) => {
        let vals: Vec<f64> = params.iter().map(|&v| v as f64).collect();
        if vals.len() % 7 != 0 {
          return Err(SvgParseError::ArcParamCount(vals.len()));
        }
        for chunk in vals.chunks(7) {
          let (rx, ry, phi) = (chunk[0], chunk[1], chunk[2]);
          let large_arc = chunk[3] != 0.0;
          let sweep = chunk[4] != 0.0;
          let (ex, ey) = (chunk[5], chunk[6]);

          let (abs_ex, abs_ey) = match pos {
            Position::Absolute => (ex, ey),
            Position::Relative => (cur_x + ex, cur_y + ey),
          };

          let new_verts = arc_vertices(
            cur_x, cur_y, rx, ry, phi, large_arc, sweep, abs_ex, abs_ey,
          );

          // Back-fill the departure angle of the vertex we are leaving.
          if let Some(first_new) = new_verts.first() {
            if let Some(last) = current.last_mut() {
              last.angle = first_new.angle;
            }
          }
          current.extend(new_verts);
          cur_x = abs_ex;
          cur_y = abs_ey;
        }
      }

      // ── Close path ────────────────────────────────────────────────────
      Command::Close => {
        // Add closing segment back to the sub-path start.
        let angle_rad = (start_y - cur_y).atan2(start_x - cur_x);
        if let Some(last) = current.last_mut() {
          last.angle = rad_to_udeg(angle_rad);
        }
        current.push(Vertex::new_with_angle(
          pt(start_x, start_y),
          rad_to_udeg(angle_rad),
        ));
        cur_x = start_x;
        cur_y = start_y;

        all.push(VertexVec(std::mem::take(&mut current)));
        has_start = false;
      }

      // ── Unsupported (Bezier curves etc.) – skip silently ──────────────
      _ => {}
    }
  }

  if !current.is_empty() {
    all.push(current);
  }

  Ok(all)
}

// ── Public API ────────────────────────────────────────────────────────────────

/// Load an SVG file from `path` and return all `<path>` elements as
/// `Vec<VertexVec>`.
///
/// Each contiguous sub-path (separated by `Move` or `Close` commands) becomes
/// one `VertexVec` entry.
///
/// Straight lines (`L`, `H`, `V`, `l`, `h`, `v`) and elliptical arcs (`A`,
/// `a`) are fully supported.  Bézier curves are skipped (the current point is
/// not advanced for them, so mixing them with lines/arcs in the same path may
/// produce gaps).
///
/// # Unit conventions
///
/// SVG user-units are treated as millimetres and converted to nanometres.
/// The `Angle` stored on each `Vertex` is the *outgoing* tangent direction of
/// the segment leaving that vertex, expressed in microdegrees CCW from the +x
/// axis.
///
/// # Errors
///
/// Returns [`SvgParseError`] on I/O failures, malformed path data, or
/// structural problems such as arc parameter counts not being a multiple of 7.
pub fn load_svg_paths<P: AsRef<Path>>(
  path: P,
) -> Result<Vec<VertexVec>, SvgParseError> {
  let mut content = String::new();
  let mut parser = svg::open(path, &mut content)?;

  collect_paths(&mut parser)
}

/// Same as [`load_svg_paths`] but parses an in-memory SVG string.
///
/// Useful for unit tests or when the SVG content is already in memory.
pub fn parse_svg_str(
  svg_content: &str,
) -> Result<Vec<VertexVec>, SvgParseError> {
  let mut parser = svg::read(svg_content)?;
  collect_paths(&mut parser)
}

fn collect_paths(parser: &mut Parser) -> Result<Vec<VertexVec>, SvgParseError> {
  let mut result = Vec::new();

  for event in parser {
    if let Event::Tag(tag::Path, _, attributes) = event {
      if let Some(d) = attributes.get("d") {
        let data =
          Data::parse(d).map_err(|e| SvgParseError::PathData(e.to_string()))?;
        let mut chains = parse_path_data(&data)?;
        result.append(&mut chains);
      }
    }
  }

  Ok(result)
}

// ── Tests ─────────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
  use super::*;

  fn nm(v: f64) -> i64 {
    uu_to_nm(v)
  }

  /// Simple horizontal line: M 0 0 L 10 0
  #[test]
  fn test_horizontal_line() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 0 0 L 10 0"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    assert_eq!(chains.len(), 1);
    let verts = &chains[0].0;
    assert_eq!(verts.len(), 2);
    assert_eq!(verts[0].pos.x.to_nm(), nm(0.0));
    assert_eq!(verts[0].pos.y.to_nm(), nm(0.0));
    // Outgoing angle from (0,0) toward (10,0) is 0 degrees.
    assert_eq!(verts[0].angle, Angle::deg_0());
    assert_eq!(verts[1].pos.x.to_nm(), nm(10.0));
  }

  /// Diagonal line: M 0 0 L 1 1  =>  outgoing angle 45 degrees.
  #[test]
  fn test_diagonal_line_angle() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 0 0 L 1 1"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    let verts = &chains[0].0;
    // 45 deg = 45_000_000 microdegrees
    assert_eq!(verts[0].angle, Angle::deg_45());
  }

  /// Closed square: M 0 0 H 10 V 10 H 0 Z
  #[test]
  fn test_closed_square() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 0 0 H 10 V 10 H 0 Z"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    assert_eq!(chains.len(), 1);
    let verts = &chains[0].0;
    // 5 vertices: (0,0) (10,0) (10,10) (0,10) (0,0)
    assert_eq!(verts.len(), 5);
    assert_eq!(verts[0].pos, verts[verts.len() - 1].pos);
  }

  /// Two separate sub-paths produced by a second Move command.
  #[test]
  fn test_two_subpaths() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 0 0 L 5 0 M 10 10 L 20 10"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    assert_eq!(chains.len(), 2);
  }

  /// Relative line command: M 5 5 l 3 4  =>  endpoint at (8, 9).
  #[test]
  fn test_relative_line() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 5 5 l 3 4"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    let last = chains[0].0.last().unwrap();
    assert_eq!(last.pos.x.to_nm(), nm(8.0));
    assert_eq!(last.pos.y.to_nm(), nm(9.0));
  }

  /// Semi-circle arc: start (10,0), r=10, large-arc, CCW, end (-10,0).
  /// The final tessellated vertex must land within 10 µm of (-10, 0).
  #[test]
  fn test_arc_semicircle_endpoint() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 10 0 A 10 10 0 1 0 -10 0"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    assert_eq!(chains.len(), 1);
    let last = chains[0].0.last().unwrap();
    let tol = uu_to_nm(0.01); // 10 µm tolerance
    assert!(
      (last.pos.x.to_nm() - nm(-10.0)).abs() < tol,
      "x off: got {} expected {}",
      last.pos.x.to_nm(),
      nm(-10.0)
    );
    assert!(
      last.pos.y.to_nm().abs() < tol,
      "y not near zero: {}",
      last.pos.y.to_nm()
    );
  }

  /// Vertical line uses correct ±90° angles.
  #[test]
  fn test_vertical_line_angles() {
    let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><path d="M 0 0 V 5 V -3"/></svg>"#;
    let chains = parse_svg_str(svg).unwrap();
    let verts = &chains[0].0;
    // segment 0->1 goes down (+y), angle = 90 deg = 90_000_000 µdeg
    assert_eq!(verts[0].angle, Angle::deg_90());
    // segment 1->2 goes up (-y), angle = -90 deg = -90_000_000 µdeg
    assert_eq!(verts[1].angle, -Angle::deg_90());
  }
}
