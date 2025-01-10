//! FFI for
//! [`interactive-html-bom::InteractiveHtmlBom`](https://docs.rs/interactive-html-bom/latest/struct.interactiveHtmlBom.html)

use super::cpp_ffi::*;
use interactive_html_bom::*;
use std::convert::From;

/// Wrapper for [ViewMode]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum InteractiveHtmlBomViewMode {
  /// BOM only
  BomOnly,
  /// BOM left, drawings right
  LeftRight,
  /// BOM top, drawings bottom
  TopBottom,
}

impl From<InteractiveHtmlBomViewMode> for ViewMode {
  fn from(val: InteractiveHtmlBomViewMode) -> Self {
    match val {
      InteractiveHtmlBomViewMode::BomOnly => ViewMode::BomOnly,
      InteractiveHtmlBomViewMode::LeftRight => ViewMode::LeftRight,
      InteractiveHtmlBomViewMode::TopBottom => ViewMode::TopBottom,
    }
  }
}

/// Wrapper for [HighlightPin1Mode]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum InteractiveHtmlBomHighlightPin1Mode {
  /// No pins
  None,
  /// Selected pins
  Selected,
  /// All pins
  All,
}

impl From<InteractiveHtmlBomHighlightPin1Mode> for HighlightPin1Mode {
  fn from(val: InteractiveHtmlBomHighlightPin1Mode) -> Self {
    match val {
      InteractiveHtmlBomHighlightPin1Mode::None => HighlightPin1Mode::None,
      InteractiveHtmlBomHighlightPin1Mode::Selected => {
        HighlightPin1Mode::Selected
      }
      InteractiveHtmlBomHighlightPin1Mode::All => HighlightPin1Mode::All,
    }
  }
}

/// Wrapper for [Layer]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum InteractiveHtmlBomLayer {
  /// Front layer
  Front,
  /// Back layer
  Back,
}

impl From<InteractiveHtmlBomLayer> for Layer {
  fn from(val: InteractiveHtmlBomLayer) -> Self {
    match val {
      InteractiveHtmlBomLayer::Front => Layer::Front,
      InteractiveHtmlBomLayer::Back => Layer::Back,
    }
  }
}

/// Wrapper for [DrawingKind]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum InteractiveHtmlBomDrawingKind {
  /// Polygon
  Polygon,
  /// Component reference designator text
  ReferenceText,
  /// Component value text
  ValueText,
}

impl From<InteractiveHtmlBomDrawingKind> for DrawingKind {
  fn from(val: InteractiveHtmlBomDrawingKind) -> Self {
    match val {
      InteractiveHtmlBomDrawingKind::Polygon => DrawingKind::Polygon,
      InteractiveHtmlBomDrawingKind::ReferenceText => {
        DrawingKind::ReferenceText
      }
      InteractiveHtmlBomDrawingKind::ValueText => DrawingKind::ValueText,
    }
  }
}

/// Wrapper for [DrawingLayer]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum InteractiveHtmlBomDrawingLayer {
  /// PCB edge
  Edge,
  /// Silkscreen front
  SilkscreenFront,
  /// Silkscreen back
  SilkscreenBack,
  /// Fabrication front
  FabricationFront,
  /// Fabrication back
  FabricationBack,
}

impl From<InteractiveHtmlBomDrawingLayer> for DrawingLayer {
  fn from(val: InteractiveHtmlBomDrawingLayer) -> Self {
    match val {
      InteractiveHtmlBomDrawingLayer::Edge => DrawingLayer::Edge,
      InteractiveHtmlBomDrawingLayer::SilkscreenFront => {
        DrawingLayer::SilkscreenFront
      }
      InteractiveHtmlBomDrawingLayer::SilkscreenBack => {
        DrawingLayer::SilkscreenBack
      }
      InteractiveHtmlBomDrawingLayer::FabricationFront => {
        DrawingLayer::FabricationFront
      }
      InteractiveHtmlBomDrawingLayer::FabricationBack => {
        DrawingLayer::FabricationBack
      }
    }
  }
}

/// Enum to define board side(s)
#[repr(C)]
#[allow(dead_code)]
enum InteractiveHtmlBomSides {
  /// Front
  Front,
  /// Back
  Back,
  /// Front + Back
  Both,
}

/// Wrapper for [Pad]
#[repr(C)]
struct InteractiveHtmlBomPad {
  /// Visible on front side
  front: bool,
  /// Visible on back side
  back: bool,
  /// Position X
  pos_x: f32,
  /// Position Y
  pos_y: f32,
  /// Rotation angle
  angle: f32,
  /// SVG path
  svgpath: *const QString,
  /// Has drill
  has_drill: bool,
  /// Drill width
  drillsize_x: f32,
  /// Drill height
  drillsize_y: f32,
  /// Net name (optional)
  net: *const QString,
  /// Pin-1 or not
  pin1: bool,
}

/// Wrapper for [RefMap]
#[repr(C)]
struct InteractiveHtmlBomRefMap {
  /// Part reference
  reference: *const QString,
  /// Footprint ID
  id: usize,
}

/// Create a new [InteractiveHtmlBom] object
#[no_mangle]
extern "C" fn ffi_ibom_new(
  title: &QString,
  company: &QString,
  revision: &QString,
  date: &QString,
  bottom_left_x: f32,
  bottom_left_y: f32,
  top_right_x: f32,
  top_right_y: f32,
) -> *mut InteractiveHtmlBom {
  let ibom = InteractiveHtmlBom::new(
    &from_qstring(title),
    &from_qstring(company),
    &from_qstring(revision),
    &from_qstring(date),
    (bottom_left_x, bottom_left_y),
    (top_right_x, top_right_y),
  );
  Box::into_raw(Box::new(ibom))
}

/// Delete [InteractiveHtmlBom] object
#[no_mangle]
extern "C" fn ffi_ibom_delete(obj: *mut InteractiveHtmlBom) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Wrapper to set [InteractiveHtmlBom::view_mode],
/// [InteractiveHtmlBom::dark_mode] and [InteractiveHtmlBom::highlight_pin1]
#[no_mangle]
extern "C" fn ffi_ibom_set_view_config(
  obj: &mut InteractiveHtmlBom,
  mode: InteractiveHtmlBomViewMode,
  highlight_pin1: InteractiveHtmlBomHighlightPin1Mode,
  dark: bool,
) {
  obj.view_mode = mode.into();
  obj.highlight_pin1 = highlight_pin1.into();
  obj.dark_mode = dark;
}

/// Wrapper to set [InteractiveHtmlBom::board_rotation] and
/// [InteractiveHtmlBom::offset_back_rotation]
#[no_mangle]
extern "C" fn ffi_ibom_set_rotation(
  obj: &mut InteractiveHtmlBom,
  angle: f32,
  offset_back: bool,
) {
  obj.board_rotation = angle;
  obj.offset_back_rotation = offset_back;
}

/// Wrapper to set [InteractiveHtmlBom::show_silkscreen]
#[no_mangle]
extern "C" fn ffi_ibom_set_show_silkscreen(
  obj: &mut InteractiveHtmlBom,
  show: bool,
) {
  obj.show_silkscreen = show;
}

/// Wrapper to set [InteractiveHtmlBom::show_fabrication]
#[no_mangle]
extern "C" fn ffi_ibom_set_show_fabrication(
  obj: &mut InteractiveHtmlBom,
  show: bool,
) {
  obj.show_fabrication = show;
}

/// Wrapper to set [InteractiveHtmlBom::show_pads]
#[no_mangle]
extern "C" fn ffi_ibom_set_show_pads(obj: &mut InteractiveHtmlBom, show: bool) {
  obj.show_pads = show;
}

/// Wrapper to set [InteractiveHtmlBom::checkboxes]
#[no_mangle]
extern "C" fn ffi_ibom_set_checkboxes(
  obj: &mut InteractiveHtmlBom,
  checkboxes: &QStringList,
) {
  obj.checkboxes = from_qstringlist(checkboxes);
}

/// Wrapper to set [InteractiveHtmlBom::fields]
#[no_mangle]
extern "C" fn ffi_ibom_set_fields(
  obj: &mut InteractiveHtmlBom,
  fields: &QStringList,
) {
  obj.fields = from_qstringlist(fields);
}

/// Wrapper for adding a drawing to [InteractiveHtmlBom::drawings]
#[no_mangle]
extern "C" fn ffi_ibom_add_drawing(
  obj: &mut InteractiveHtmlBom,
  kind: InteractiveHtmlBomDrawingKind,
  layer: InteractiveHtmlBomDrawingLayer,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.drawings.push(Drawing::new(
    kind.into(),
    layer.into(),
    &from_qstring(svgpath),
    width,
    filled,
  ));
}

/// Wrapper for [InteractiveHtmlBom::add_footprint]
#[no_mangle]
extern "C" fn ffi_ibom_add_footprint(
  obj: &mut InteractiveHtmlBom,
  layer: InteractiveHtmlBomLayer,
  pos_x: f32,
  pos_y: f32,
  angle: f32,
  bottom_left_x: f32,
  bottom_left_y: f32,
  top_right_x: f32,
  top_right_y: f32,
  mount: bool,
  fields: &QStringList,
  pads_array: *const InteractiveHtmlBomPad,
  pads_size: usize,
) -> usize {
  let mut pads = Vec::new();
  unsafe {
    for pad in std::slice::from_raw_parts(pads_array, pads_size) {
      let mut layers = Vec::new();
      if pad.front {
        layers.push(Layer::Front);
      }
      if pad.back {
        layers.push(Layer::Back);
      }
      let net = if pad.net.is_null() {
        None
      } else {
        Some(from_qstring(&*pad.net))
      };
      pads.push(Pad::new(
        &layers,
        (pad.pos_x, pad.pos_y),
        pad.angle,
        &from_qstring(&*pad.svgpath),
        if pad.has_drill {
          Some((pad.drillsize_x, pad.drillsize_y))
        } else {
          None
        },
        net.as_deref(),
        pad.pin1,
      ));
    }
  }

  obj.add_footprint(Footprint::new(
    layer.into(),
    (pos_x, pos_y),
    angle,
    (bottom_left_x, bottom_left_y),
    (top_right_x, top_right_y),
    &from_qstringlist(fields),
    &pads,
    mount,
  ))
}

/// Wrapper to add BOM lines to [InteractiveHtmlBom::bom_front],
/// [InteractiveHtmlBom::bom_back] or [InteractiveHtmlBom::bom_both]
#[no_mangle]
extern "C" fn ffi_ibom_add_bom_line(
  obj: &mut InteractiveHtmlBom,
  sides: InteractiveHtmlBomSides,
  parts_array: *const InteractiveHtmlBomRefMap,
  parts_size: usize,
) {
  let mut vec = Vec::new();
  unsafe {
    for map in std::slice::from_raw_parts(parts_array, parts_size) {
      vec.push(RefMap::new(&from_qstring(&*map.reference), map.id));
    }
  }

  match sides {
    InteractiveHtmlBomSides::Front => obj.bom_front.push(vec),
    InteractiveHtmlBomSides::Back => obj.bom_back.push(vec),
    InteractiveHtmlBomSides::Both => obj.bom_both.push(vec),
  }
}

/// Wrapper for adding a track to [InteractiveHtmlBom::tracks]
#[no_mangle]
extern "C" fn ffi_ibom_add_track(
  obj: &mut InteractiveHtmlBom,
  layer: InteractiveHtmlBomLayer,
  start_x: f32,
  start_y: f32,
  end_x: f32,
  end_y: f32,
  width: f32,
  net_name: *const QString,
) {
  let net = if net_name.is_null() {
    None
  } else {
    unsafe { Some(from_qstring(&*net_name)) }
  };
  obj.tracks.push(Track::new(
    layer.into(),
    (start_x, start_y),
    (end_x, end_y),
    width,
    net.as_deref(),
  ));
}

/// Wrapper for adding a via to [InteractiveHtmlBom::vias]
#[no_mangle]
extern "C" fn ffi_ibom_add_via(
  obj: &mut InteractiveHtmlBom,
  layers_array: *const InteractiveHtmlBomLayer,
  layers_size: usize,
  pos_x: f32,
  pos_y: f32,
  diameter: f32,
  drill_diameter: f32,
  net_name: *const QString,
) {
  unsafe {
    let layers = std::slice::from_raw_parts(layers_array, layers_size)
      .iter()
      .map(|x| (*x).into())
      .collect::<Vec<_>>();
    let net = if net_name.is_null() {
      None
    } else {
      Some(from_qstring(&*net_name))
    };
    obj.vias.push(Via::new(
      &layers,
      (pos_x, pos_y),
      diameter,
      drill_diameter,
      net.as_deref(),
    ));
  }
}

/// Wrapper for adding a zone to [InteractiveHtmlBom::zones]
#[no_mangle]
extern "C" fn ffi_ibom_add_zone(
  obj: &mut InteractiveHtmlBom,
  layer: InteractiveHtmlBomLayer,
  svgpath: &QString,
  net_name: *const QString,
) {
  let net = if net_name.is_null() {
    None
  } else {
    unsafe { Some(from_qstring(&*net_name)) }
  };
  obj.zones.push(Zone::new(
    layer.into(),
    &from_qstring(svgpath),
    net.as_deref(),
  ));
}

/// Wrapper for [InteractiveHtmlBom::generate_html]
#[no_mangle]
extern "C" fn ffi_ibom_generate_html(
  obj: &InteractiveHtmlBom,
  out: &mut QString,
  err: &mut QString,
) -> bool {
  match obj.generate_html() {
    Ok(html) => {
      qstring_set(out, &html);
      true
    }
    Err(msg) => {
      qstring_set(err, &msg);
      false
    }
  }
}
