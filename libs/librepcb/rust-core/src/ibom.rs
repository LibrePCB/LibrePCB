//! Ineractive HTML BOM generator
//!
//! See JSON format documentation here:
//! https://github.com/openscopeproject/InteractiveHtmlBom/blob/master/DATAFORMAT.md

use jzon::{array, object, JsonValue};

trait Serializable {
  fn to_json(&self) -> JsonValue;
}

impl<T: Serializable> Serializable for Vec<T> {
  fn to_json(&self) -> JsonValue {
    let mut arr = array![];
    for item in self {
      arr.push(item.to_json()).unwrap();
    }
    arr
  }
}

pub struct Coordinate {
  x: f32,
  y: f32,
}

impl Coordinate {
  pub fn new(x: f32, y: f32) -> Coordinate {
    Coordinate { x, y }
  }
}

impl Serializable for Coordinate {
  fn to_json(&self) -> JsonValue {
    array![self.x, self.y]
  }
}

pub struct BoundingBox {
  min_x: f32,
  max_x: f32,
  min_y: f32,
  max_y: f32,
}

impl BoundingBox {
  pub fn new(min_x: f32, max_x: f32, min_y: f32, max_y: f32) -> BoundingBox {
    BoundingBox {
      min_x,
      max_x,
      min_y,
      max_y,
    }
  }
}

impl Serializable for BoundingBox {
  fn to_json(&self) -> JsonValue {
    object! {
      minx: self.min_x,
      maxx: self.max_x,
      miny: self.min_y,
      maxy: self.max_y,
    }
  }
}

//pub struct DrawingSegment {
//  start: Coordinate,
//  end: Coordinate,
//  width: f32,
//}
//
//impl DrawingSegment {
//  pub fn new(start: Coordinate, end: Coordinate, width: f32) -> DrawingSegment {
//    DrawingSegment { start, end, width }
//  }
//}
//
//impl Serializable for DrawingSegment {
//  fn to_json(&self) -> JsonValue {
//    object! {
//      type: "segment",
//      start: self.start.to_json(),
//      end: self.end.to_json(),
//      width: self.width,
//    }
//  }
//}

//pub enum Drawing {
//  Segment(DrawingSegment),
//}

//impl Serializable for Drawing {
//  fn to_json(&self) -> JsonValue {
//    match self {
//      Drawing::Segment(obj) => obj.to_json(),
//    }
//  }
//}

pub struct Drawing {
  svgpath: String,
  filled: bool,
  width: f32,
}

impl Drawing {
  pub fn new(svgpath: String, width: f32, filled: bool) -> Drawing {
    Drawing {
      svgpath,
      width,
      filled,
    }
  }
}

impl Serializable for Drawing {
  fn to_json(&self) -> JsonValue {
    object! {
      type: "polygon",
      svgpath: self.svgpath.clone(),
      width: self.width,
      filled: self.filled,
    }
  }
}

pub enum Layer {
  Front,
  Back,
}

impl Serializable for Layer {
  fn to_json(&self) -> JsonValue {
    match self {
      Layer::Front => "F".into(),
      Layer::Back => "B".into(),
    }
  }
}

pub struct Footprint {
  reference: String,
  layer: Layer,
  pos: Coordinate,
  angle: f32,
  relpos: Coordinate,
  size: Coordinate,
}

impl Footprint {
  pub fn new(
    reference: String,
    layer: Layer,
    pos: Coordinate,
    angle: f32,
    relpos: Coordinate,
    size: Coordinate,
  ) -> Footprint {
    Footprint {
      reference,
      layer,
      pos,
      angle,
      relpos,
      size,
    }
  }
}

impl Serializable for Footprint {
  fn to_json(&self) -> JsonValue {
    object! {
      ref: self.reference.clone(),
      bbox: object!{
        pos: self.pos.to_json(),
        angle: self.angle,
        relpos: self.relpos.to_json(),
        size: self.size.to_json(),
      },
      drawings: array![],
      layer: self.layer.to_json(),
      pads: array![],
    }
  }
}

#[derive(Clone)]
pub struct RefMap {
  reference: String,
  footprint_id: usize,
}

impl RefMap {
  pub fn new(reference: String, footprint_id: usize) -> RefMap {
    RefMap {
      reference,
      footprint_id,
    }
  }
}

impl Serializable for RefMap {
  fn to_json(&self) -> JsonValue {
    array! {
      self.reference.clone(),
      self.footprint_id,
    }
  }
}

pub struct InteractiveHtmlBom {
  title: String,
  revision: String,
  company: String,
  date: String,
  bbox: BoundingBox,
  edges: Vec<Drawing>,
  silkscreen_front: Vec<Drawing>,
  silkscreen_back: Vec<Drawing>,
  fabrication_front: Vec<Drawing>,
  fabrication_back: Vec<Drawing>,
  footprints: Vec<Footprint>,
  bom_front: Vec<Vec<RefMap>>,
  bom_back: Vec<Vec<RefMap>>,
}

impl InteractiveHtmlBom {
  pub fn new(
    title: String,
    revision: String,
    company: String,
    date: String,
    bbox: BoundingBox,
  ) -> InteractiveHtmlBom {
    InteractiveHtmlBom {
      title,
      revision,
      company,
      date,
      bbox,
      edges: Vec::new(),
      silkscreen_front: Vec::new(),
      silkscreen_back: Vec::new(),
      fabrication_front: Vec::new(),
      fabrication_back: Vec::new(),
      footprints: Vec::new(),
      bom_front: Vec::new(),
      bom_back: Vec::new(),
    }
  }

  pub fn add_edge(&mut self, d: Drawing) {
    self.edges.push(d);
  }

  pub fn add_silkscreen_front(&mut self, d: Drawing) {
    self.silkscreen_front.push(d);
  }

  pub fn add_silkscreen_back(&mut self, d: Drawing) {
    self.silkscreen_back.push(d);
  }

  pub fn add_fabrication_front(&mut self, d: Drawing) {
    self.fabrication_front.push(d);
  }

  pub fn add_fabrication_back(&mut self, d: Drawing) {
    self.fabrication_back.push(d);
  }

  pub fn add_footprint(&mut self, f: Footprint) -> usize {
    self.footprints.push(f);
    self.footprints.len() - 1
  }

  pub fn add_bom_line(&mut self, layer: Layer, b: Vec<RefMap>) {
    match layer {
      Layer::Front => self.bom_front.push(b),
      Layer::Back => self.bom_back.push(b),
    }
  }

  pub fn generate(&self) -> String {
    let config = r#"var config = {
        "show_fabrication": true,
        "redraw_on_drag": true,
        "highlight_pin1": "none",
        "offset_back_rotation": false,
        "kicad_text_formatting": true,
        "dark_mode": false,
        "bom_view": "left-right",
        "board_rotation": 0.0,
        "checkboxes": "Sourced,Placed",
        "show_silkscreen": true,
        "fields": [
        ],
        "show_pads": true,
        "layer_view": "FB"
    }"#;

    let bom_both = [&self.bom_front[..], &self.bom_back[..]].concat();

    let mut data = object! {};
    data["ibom_version"] = "v2.9.0".into();
    data["metadata"]["title"] = self.title.clone().into();
    data["metadata"]["revision"] = self.revision.clone().into();
    data["metadata"]["company"] = self.company.clone().into();
    data["metadata"]["date"] = self.date.clone().into();
    data["bom"]["F"] = self.bom_front.to_json();
    data["bom"]["B"] = self.bom_back.to_json();
    data["bom"]["both"] = bom_both.to_json();
    data["bom"]["skipped"] = array! {};
    data["bom"]["fields"] = object! {};
    for row in &bom_both {
      for map in row {
        data["bom"]["fields"][map.footprint_id.to_string()] = object! {};
      }
    }
    data["edges_bbox"] = self.bbox.to_json();
    data["footprints"] = self.footprints.to_json();
    data["edges"] = self.edges.to_json();
    data["drawings"]["silkscreen"]["F"] = self.silkscreen_front.to_json();
    data["drawings"]["silkscreen"]["B"] = self.silkscreen_back.to_json();
    data["drawings"]["fabrication"]["F"] = self.fabrication_front.to_json();
    data["drawings"]["fabrication"]["B"] = self.fabrication_back.to_json();
    let data_compressed = lz_str::compress_to_base64(&data.dump());
    let pcbdata = "var pcbdata = JSON.parse(LZString.decompressFromBase64(\""
      .to_owned()
      + &data_compressed
      + "\"))";

    let mut html =
      String::from_utf8_lossy(include_bytes!("ibom/web/ibom.html")).to_string();
    html = html.replace(
      "///CSS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/ibom.css")),
    );
    html = html.replace(
      "///SPLITJS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/split.js")),
    );
    html = html.replace(
      "///LZ-STRING///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/lz-string.js")),
    );
    html = html.replace(
      "///POINTER_EVENTS_POLYFILL///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/pep.js")),
    );
    html = html.replace("///CONFIG///", config);
    html = html.replace("///PCBDATA///", &pcbdata);
    html = html.replace(
      "///UTILJS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/util.js")),
    );
    html = html.replace(
      "///RENDERJS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/render.js")),
    );
    html = html.replace(
      "///TABLEUTILJS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/table-util.js")),
    );
    html = html.replace(
      "///IBOMJS///",
      &String::from_utf8_lossy(include_bytes!("ibom/web/ibom.js")),
    );
    html = html.replace("///USERJS///", "");
    html = html.replace("///USERHEADER///", "");
    html = html.replace("///USERFOOTER///", "");
    html
  }
}
