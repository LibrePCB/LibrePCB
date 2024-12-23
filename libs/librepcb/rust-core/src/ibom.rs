//! Ineractive HTML BOM generator
//!
//! See JSON format documentation here:
//! https://github.com/openscopeproject/InteractiveHtmlBom/blob/master/DATAFORMAT.md

use jzon::{array, object};

pub struct InteractiveHtmlBom {
  title: String,
  revision: String,
  company: String,
  date: String,
}

impl InteractiveHtmlBom {
  pub fn new(
    title: String,
    revision: String,
    company: String,
    date: String,
  ) -> InteractiveHtmlBom {
    InteractiveHtmlBom {
      title,
      revision,
      company,
      date,
    }
  }

  pub fn generate(&self) -> String {
    let config = r#"var config = {
        "show_fabrication": false,
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
            "Value",
            "Footprint"
        ],
        "show_pads": true,
        "layer_view": "FB"
    }"#;

    let mut data = object! {};
    data["ibom_version"] = "v2.9.0".into();
    data["metadata"]["title"] = self.title.clone().into();
    data["metadata"]["revision"] = self.revision.clone().into();
    data["metadata"]["company"] = self.company.clone().into();
    data["metadata"]["date"] = self.date.clone().into();
    data["bom"]["both"] = array! {};
    data["bom"]["F"] = array! {};
    data["bom"]["B"] = array! {};
    data["bom"]["skipped"] = array! {};
    data["bom"]["fields"] = object! {};
    data["edges_bbox"]["minx"] = 0.into();
    data["edges_bbox"]["miny"] = 0.into();
    data["edges_bbox"]["maxx"] = 100.into();
    data["edges_bbox"]["maxy"] = 100.into();
    data["footprints"] = array![];
    data["edges"] = array![];
    data["drawings"]["silkscreen"]["F"] = array! {};
    data["drawings"]["silkscreen"]["B"] = array! {};
    data["drawings"]["fabrication"]["F"] = array! {};
    data["drawings"]["fabrication"]["B"] = array! {};
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
