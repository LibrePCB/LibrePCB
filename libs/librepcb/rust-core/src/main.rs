//use librepcb_rust_core::svg_reader;

//use librepcb_rust_core::{Angle, Length, Point, Vertex};
//use std::vec::Vec;
//use svg::node::element::path::{Command, Data};
//use svg::node::element::tag::Path;
//use svg::parser::Event;

//struct Vertices(Vec<Vertex>);

fn main() {
  /*let path = "/home/urban/Downloads/Font-Awesome-6.x/js-packages/@fortawesome/fontawesome-free/svgs/solid/1.svg";
  let mut content = String::new();
  for event in svg::open(path, &mut content).unwrap() {
    match event {
      Event::Tag(Path, _, attributes) => {
        let data = attributes.get("d").unwrap();
        let data = Data::parse(data).unwrap();
        for command in data.iter() {
          match command {
            &Command::Move(p, ref params) => {
              println!("Move {:?}", params.to_vec());
            }
            &Command::Line(p, ref params) => {
              println!("Line {:?}", params.to_vec());
            }
            _ => {}
          }
        }
      }
      _ => {
        println!("Foo");
      }
    }
  }*/

  println!("Hello, world!");
}
