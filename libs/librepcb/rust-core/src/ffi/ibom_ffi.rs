//! FFI for [ibom][crate::ibom]

use super::cpp_ffi::*;
use crate::ibom::*;

#[repr(C)]
pub enum InteractiveHtmlBomLayer {
  Front,
  Back,
}

impl InteractiveHtmlBomLayer {
  fn convert(&self) -> Layer {
    match self {
      InteractiveHtmlBomLayer::Front => Layer::Front,
      InteractiveHtmlBomLayer::Back => Layer::Back,
    }
  }
}

#[repr(C)]
struct InteractiveHtmlBomRefMap {
  reference: *const QString,
  id: usize,
}

/// Create a new [InteractiveHtmlBom] object
#[no_mangle]
pub extern "C" fn ffi_ibom_new(
  title: &QString,
  revision: &QString,
  company: &QString,
  date: &QString,
  min_x: f32,
  max_x: f32,
  min_y: f32,
  max_y: f32,
) -> *mut InteractiveHtmlBom {
  let ibom = InteractiveHtmlBom::new(
    from_qstring(title),
    from_qstring(revision),
    from_qstring(company),
    from_qstring(date),
    BoundingBox::new(min_x, max_x, min_y, max_y),
  );
  Box::into_raw(Box::new(ibom))
}

/// Delete [InteractiveHtmlBom] object
#[no_mangle]
pub extern "C" fn ffi_ibom_delete(obj: *mut InteractiveHtmlBom) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Wrapper for [add_edge]
#[no_mangle]
extern "C" fn ffi_ibom_add_edge(
  obj: &mut InteractiveHtmlBom,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.add_edge(Drawing::new(from_qstring(svgpath), width, filled));
}

/// Wrapper for [add_silkscreen_front]
#[no_mangle]
extern "C" fn ffi_ibom_add_silkscreen_front(
  obj: &mut InteractiveHtmlBom,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.add_silkscreen_front(Drawing::new(from_qstring(svgpath), width, filled));
}

/// Wrapper for [add_silkscreen_back]
#[no_mangle]
extern "C" fn ffi_ibom_add_silkscreen_back(
  obj: &mut InteractiveHtmlBom,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.add_silkscreen_back(Drawing::new(from_qstring(svgpath), width, filled));
}

/// Wrapper for [add_fabrication_front]
#[no_mangle]
extern "C" fn ffi_ibom_add_fabrication_front(
  obj: &mut InteractiveHtmlBom,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.add_fabrication_front(Drawing::new(from_qstring(svgpath), width, filled));
}

/// Wrapper for [add_fabrication_back]
#[no_mangle]
extern "C" fn ffi_ibom_add_fabrication_back(
  obj: &mut InteractiveHtmlBom,
  svgpath: &QString,
  width: f32,
  filled: bool,
) {
  obj.add_fabrication_back(Drawing::new(from_qstring(svgpath), width, filled));
}

/// Wrapper for [add_footprint]
#[no_mangle]
extern "C" fn ffi_ibom_add_footprint(
  obj: &mut InteractiveHtmlBom,
  reference: &QString,
  layer: InteractiveHtmlBomLayer,
  pos_x: f32,
  pos_y: f32,
  angle: f32,
  relpos_x: f32,
  relpos_y: f32,
  size_x: f32,
  size_y: f32,
) -> usize {
  obj.add_footprint(Footprint::new(
    from_qstring(reference),
    layer.convert(),
    Coordinate::new(pos_x, pos_y),
    angle,
    Coordinate::new(relpos_x, relpos_y),
    Coordinate::new(size_x, size_y),
  ))
}

/// Wrapper for [add_bom_both]
#[no_mangle]
extern "C" fn ffi_ibom_add_bom_line(
  obj: &mut InteractiveHtmlBom,
  layer: InteractiveHtmlBomLayer,
  parts: *const InteractiveHtmlBomRefMap,
  size: usize,
) {
  let mut vec = Vec::new();
  unsafe {
    for map in std::slice::from_raw_parts(parts, size) {
      vec.push(RefMap::new(from_qstring(&*map.reference), map.id));
    }
  }
  obj.add_bom_line(layer.convert(), vec);
}

/// Wrapper for [generate]
#[no_mangle]
extern "C" fn ffi_ibom_generate(obj: &InteractiveHtmlBom, out: &mut QString) {
  let r_str = obj.generate();
  qstring_set(out, &r_str);
}
