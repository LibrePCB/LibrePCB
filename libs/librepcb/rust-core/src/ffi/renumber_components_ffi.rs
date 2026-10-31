//! FFI for [renumber_components][crate::renumber_components].

use super::cpp_ffi::*;
use crate::renumber_components::*;
use crate::{Length, Point};

/// Symbol data for [ffi_renumber_components].
#[repr(C)]
pub struct RenumberComponentsSymbolData {
  /// Index of the component this symbol belongs to.
  pub component: usize,
  /// Index of the schematic containing this symbol.
  pub schematic: usize,
  /// X position in nanometers.
  pub x: i64,
  /// Y position in nanometers.
  pub y: i64,
}

/// Wrapper for [renumber_components].
///
/// Connections are passed as `(symbol index, net segment ID)` pairs, i.e.
/// `connections_len` must be a multiple of 2. The new designators (parallel
/// to `names`) are appended to `out`.
#[no_mangle]
extern "C" fn ffi_renumber_components(
  names: &QStringList,
  symbols: *const RenumberComponentsSymbolData,
  symbols_len: usize,
  connections: *const usize,
  connections_len: usize,
  out: &mut QStringList,
) {
  assert!(!symbols.is_null());
  assert!(!connections.is_null());
  unsafe {
    let symbols: Vec<RenumberComponentsSymbol> =
      std::slice::from_raw_parts(symbols, symbols_len)
        .iter()
        .map(|s| RenumberComponentsSymbol {
          component: s.component,
          schematic: s.schematic,
          position: Point::new(Length::from_nm(s.x), Length::from_nm(s.y)),
        })
        .collect();
    let connections: Vec<(usize, usize)> =
      std::slice::from_raw_parts(connections, connections_len)
        .chunks_exact(2)
        .map(|c| (c[0], c[1]))
        .collect();
    let names = from_qstringlist(names);
    let names: Vec<&str> = names.iter().map(String::as_str).collect();
    for name in renumber_components(&names, &symbols, &connections) {
      qstringlist_append(out, &name);
    }
  }
}
