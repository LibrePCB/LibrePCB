//! FFI for [ibom][crate::ibom]

use super::cpp_ffi::*;
use crate::ibom::*;

#[repr(C)]
#[allow(dead_code)]
enum InteractiveHtmlBomLayer {
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
#[allow(dead_code)]
enum InteractiveHtmlBomSides {
  Front,
  Back,
  Both,
}

impl InteractiveHtmlBomSides {
  fn convert(&self) -> Sides {
    match self {
      InteractiveHtmlBomSides::Front => Sides::Front,
      InteractiveHtmlBomSides::Back => Sides::Back,
      InteractiveHtmlBomSides::Both => Sides::Both,
    }
  }
}

#[repr(C)]
struct InteractiveHtmlBomRefMap {
  reference: *const QString,
  id: usize,
}

#[repr(C)]
struct InteractiveHtmlBomPad {
  front: bool,
  back: bool,
  pos_x: f32,
  pos_y: f32,
  angle: f32,
  svgpath: *const QString,
  has_drill: bool,
  drillsize_x: f32,
  drillsize_y: f32,
  netname: *const QString,
}

/// Create a new [InteractiveHtmlBom] object
#[no_mangle]
extern "C" fn ffi_ibom_new(
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
extern "C" fn ffi_ibom_delete(obj: *mut InteractiveHtmlBom) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Wrapper for [set_fields]
#[no_mangle]
extern "C" fn ffi_ibom_set_fields(
  obj: &mut InteractiveHtmlBom,
  fields: &QStringList,
) {
  obj.set_fields(from_qstringlist(fields));
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
      let net = if pad.netname.is_null() {
        None
      } else {
        Some(from_qstring(&*pad.netname))
      };
      pads.push(Pad::new(
        layers,
        Coordinate::new(pad.pos_x, pad.pos_y),
        pad.angle,
        from_qstring(&*pad.svgpath),
        if pad.has_drill {
          Some((pad.drillsize_x, pad.drillsize_y))
        } else {
          None
        },
        net,
      ));
    }
  }

  obj.add_footprint(
    Footprint::new(
      from_qstring(reference),
      layer.convert(),
      Coordinate::new(pos_x, pos_y),
      angle,
      Coordinate::new(relpos_x, relpos_y),
      Coordinate::new(size_x, size_y),
      from_qstringlist(fields),
      pads,
    ),
    mount,
  )
}

/// Wrapper for [add_bom_line]
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
      vec.push(RefMap::new(from_qstring(&*map.reference), map.id));
    }
  }
  obj.add_bom_line(sides.convert(), vec);
}

/// Wrapper for [add_track]
#[no_mangle]
extern "C" fn ffi_ibom_add_track(
  obj: &mut InteractiveHtmlBom,
  layer: InteractiveHtmlBomLayer,
  start_x: f32,
  start_y: f32,
  end_x: f32,
  end_y: f32,
  width: f32,
  drillsize: Option<&f32>,
  net_name: *const QString,
) {
  let net = if net_name.is_null() {
    None
  } else {
    unsafe { Some(from_qstring(&*net_name)) }
  };
  obj.add_track(
    layer.convert(),
    Track::new(
      Coordinate::new(start_x, start_y),
      Coordinate::new(end_x, end_y),
      width,
      drillsize.copied(),
      net,
    ),
  );
}

/// Wrapper for [add_zone]
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
  obj.add_zone(layer.convert(), Zone::new(from_qstring(svgpath), net));
}

/// Wrapper for [generate]
#[no_mangle]
extern "C" fn ffi_ibom_generate(
  obj: &InteractiveHtmlBom,
  out: &mut QString,
  err: &mut QString,
) -> bool {
  match obj.generate() {
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
