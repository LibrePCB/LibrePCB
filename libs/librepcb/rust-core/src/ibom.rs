//! Ineractive HTML BOM generator
//!
//! See JSON format documentation here:
//! https://github.com/openscopeproject/InteractiveHtmlBom/blob/master/DATAFORMAT.md

pub struct InteractiveHtmlBom {}

impl InteractiveHtmlBom {
  pub fn new() -> InteractiveHtmlBom {
    InteractiveHtmlBom {}
  }

  pub fn generate(&self) -> String {
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
    //html = html.replace(
    //  "///CONFIG///",
    //  "",
    //);
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
    //html = html.replace(
    //  "///PCBDATA///",
    //  "",
    //);

    html
  }
}
