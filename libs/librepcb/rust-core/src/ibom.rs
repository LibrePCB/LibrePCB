//! Ineractive HTML BOM generator
//!
//! See JSON format documentation here:
//! https://github.com/openscopeproject/InteractiveHtmlBom/blob/master/DATAFORMAT.md

use jzon::{array, object, JsonValue};

trait Serializable {
  fn to_json(&self) -> JsonValue;
}

impl Serializable for usize {
  fn to_json(&self) -> JsonValue {
    (*self).into()
  }
}

impl Serializable for String {
  fn to_json(&self) -> JsonValue {
    self.clone().into()
  }
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

pub struct Track {
  start: Coordinate,
  end: Coordinate,
  width: f32,
  drillsize: Option<f32>,
  net: Option<String>,
}

impl Track {
  pub fn new(
    start: Coordinate,
    end: Coordinate,
    width: f32,
    drillsize: Option<f32>,
    net: Option<String>,
  ) -> Track {
    Track {
      start,
      end,
      width,
      drillsize,
      net,
    }
  }
}

impl Serializable for Track {
  fn to_json(&self) -> JsonValue {
    let mut obj = object! {
      start: self.start.to_json(),
      end: self.end.to_json(),
      width: self.width,
    };
    if self.drillsize.is_some() {
      obj["drillsize"] = self.drillsize.unwrap().into();
    }
    if let Some(net) = &self.net {
      obj["net"] = net.clone().into();
    }
    obj
  }
}

pub struct Zone {
  svgpath: String,
  net: Option<String>,
}

impl Zone {
  pub fn new(svgpath: String, net: Option<String>) -> Zone {
    Zone { svgpath, net }
  }
}

impl Serializable for Zone {
  fn to_json(&self) -> JsonValue {
    let mut obj = object! {
      svgpath: self.svgpath.clone(),
    };
    if let Some(net) = &self.net {
      obj["net"] = net.clone().into();
    }
    obj
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

pub enum Sides {
  Front,
  Back,
  Both,
}

pub struct Pad {
  layers: Vec<Layer>,
  pos: Coordinate,
  angle: f32,
  svgpath: String,
  drillsize: Option<(f32, f32)>,
  net: Option<String>,
}

impl Pad {
  pub fn new(
    layers: Vec<Layer>,
    pos: Coordinate,
    angle: f32,
    svgpath: String,
    drillsize: Option<(f32, f32)>,
    net: Option<String>,
  ) -> Pad {
    Pad {
      layers,
      pos,
      angle,
      svgpath,
      drillsize,
      net,
    }
  }
}

impl Serializable for Pad {
  fn to_json(&self) -> JsonValue {
    let mut obj = object! {
      layers: self.layers.to_json(),
      pos: self.pos.to_json(),
      angle: self.angle,
      shape: "custom",
      svgpath: self.svgpath.clone(),
    };
    if let Some(drill) = &self.drillsize {
      obj["type"] = "th".into();
      obj["drillsize"] = array![drill.0, drill.1];
      obj["drillshape"] = if drill.0 != drill.1 {
        "oblong"
      } else {
        "circle"
      }
      .into();
    } else {
      obj["type"] = "smd".into();
    }
    if let Some(net) = &self.net {
      obj["net"] = net.clone().into();
    }
    obj
  }
}

pub struct Footprint {
  layer: Layer,
  pos: Coordinate,
  angle: f32,
  relpos: Coordinate,
  size: Coordinate,
  fields: Vec<String>,
  pads: Vec<Pad>,
}

impl Footprint {
  pub fn new(
    layer: Layer,
    pos: Coordinate,
    angle: f32,
    relpos: Coordinate,
    size: Coordinate,
    fields: Vec<String>,
    pads: Vec<Pad>,
  ) -> Footprint {
    Footprint {
      layer,
      pos,
      angle,
      relpos,
      size,
      fields,
      pads,
    }
  }
}

impl Serializable for Footprint {
  fn to_json(&self) -> JsonValue {
    object! {
      bbox: object!{
        pos: self.pos.to_json(),
        angle: self.angle,
        relpos: self.relpos.to_json(),
        size: self.size.to_json(),
      },
      drawings: array![],
      layer: self.layer.to_json(),
      pads: self.pads.to_json(),
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
  // Metadata
  title: String,
  revision: String,
  company: String,
  date: String,
  bbox: BoundingBox,

  // Config
  show_fabrication: bool,
  show_silkscreen: bool,
  fields: Vec<String>,

  // Content
  edges: Vec<Drawing>,
  silkscreen_front: Vec<Drawing>,
  silkscreen_back: Vec<Drawing>,
  fabrication_front: Vec<Drawing>,
  fabrication_back: Vec<Drawing>,
  footprints: Vec<Footprint>,
  bom_front: Vec<Vec<RefMap>>,
  bom_back: Vec<Vec<RefMap>>,
  bom_both: Vec<Vec<RefMap>>,
  bom_skipped: Vec<usize>,
  tracks_front: Vec<Track>,
  tracks_back: Vec<Track>,
  zones_front: Vec<Zone>,
  zones_back: Vec<Zone>,
  nets: Vec<String>,
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
      show_fabrication: true,
      show_silkscreen: true,
      fields: Vec::new(),
      edges: Vec::new(),
      silkscreen_front: Vec::new(),
      silkscreen_back: Vec::new(),
      fabrication_front: Vec::new(),
      fabrication_back: Vec::new(),
      footprints: Vec::new(),
      bom_front: Vec::new(),
      bom_back: Vec::new(),
      bom_both: Vec::new(),
      bom_skipped: Vec::new(),
      tracks_front: Vec::new(),
      tracks_back: Vec::new(),
      zones_front: Vec::new(),
      zones_back: Vec::new(),
      nets: Vec::new(),
    }
  }

  pub fn set_fields(&mut self, f: Vec<String>) {
    self.fields = f;
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

  pub fn add_footprint(&mut self, f: Footprint, mount: bool) -> usize {
    for pad in &f.pads {
      if let Some(net) = &pad.net {
        if !self.nets.contains(&net) {
          self.nets.push(net.clone());
        }
      }
    }
    let id = self.footprints.len();
    self.footprints.push(f);
    if !mount {
      self.bom_skipped.push(id);
    }
    id
  }

  pub fn add_bom_line(&mut self, sides: Sides, b: Vec<RefMap>) {
    match sides {
      Sides::Front => self.bom_front.push(b),
      Sides::Back => self.bom_back.push(b),
      Sides::Both => self.bom_both.push(b),
    }
  }

  pub fn add_track(&mut self, layer: Layer, t: Track) {
    match layer {
      Layer::Front => self.tracks_front.push(t),
      Layer::Back => self.tracks_back.push(t),
    }
  }

  pub fn add_zone(&mut self, layer: Layer, z: Zone) {
    match layer {
      Layer::Front => self.zones_front.push(z),
      Layer::Back => self.zones_back.push(z),
    }
  }

  pub fn generate(&self) -> Result<String, String> {
    let config = object! {
        board_rotation: 0.0,
        bom_view: "left-right",
        checkboxes: "Sourced,Placed",
        dark_mode: false,
        fields: self.fields.to_json(),
        highlight_pin1: "none",
        kicad_text_formatting: true,
        layer_view: "FB",
        offset_back_rotation: false,
        redraw_on_drag: true,
        show_fabrication: self.show_fabrication,
        show_pads: true,
        show_silkscreen: self.show_silkscreen,
    };
    let config_str = "var config = ".to_owned() + &config.dump();

    let mut data = object! {};
    data["ibom_version"] = "v2.9.0".into();
    data["metadata"]["title"] = self.title.clone().into();
    data["metadata"]["revision"] = self.revision.clone().into();
    data["metadata"]["company"] = self.company.clone().into();
    data["metadata"]["date"] = self.date.clone().into();
    data["bom"]["F"] = self.bom_front.to_json();
    data["bom"]["B"] = self.bom_back.to_json();
    data["bom"]["both"] = self.bom_both.to_json();
    data["bom"]["skipped"] = self.bom_skipped.to_json();
    data["bom"]["fields"] = object! {};
    for (id, fpt) in self.footprints.iter().enumerate() {
      if fpt.fields.len() != self.fields.len() {
        return Err("Inconsistent number of fields.".into());
      }
      data["bom"]["fields"][id.to_string()] = fpt.fields.to_json();
    }
    data["edges_bbox"] = self.bbox.to_json();
    data["footprints"] = self.footprints.to_json();
    data["edges"] = self.edges.to_json();
    data["drawings"]["silkscreen"]["F"] = self.silkscreen_front.to_json();
    data["drawings"]["silkscreen"]["B"] = self.silkscreen_back.to_json();
    data["drawings"]["fabrication"]["F"] = self.fabrication_front.to_json();
    data["drawings"]["fabrication"]["B"] = self.fabrication_back.to_json();
    data["tracks"]["F"] = self.tracks_front.to_json();
    data["tracks"]["B"] = self.tracks_back.to_json();
    data["zones"]["F"] = self.zones_front.to_json();
    data["zones"]["B"] = self.zones_back.to_json();
    data["nets"] = self.nets.to_json();
    let data_compressed = lz_str::compress_to_base64(&data.dump());
    let pcbdata_str =
      "var pcbdata = JSON.parse(LZString.decompressFromBase64(\"".to_owned()
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
    html = html.replace("///CONFIG///", &config_str);
    html = html.replace("///PCBDATA///", &pcbdata_str);
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
    Ok(html)
  }
}
