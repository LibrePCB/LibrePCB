//! Algorithm to determine natural component designators ("renumbering").

use crate::{Length, Point};
use std::collections::HashMap;

/// Position and connectivity of a component symbol in a schematic.
pub struct RenumberComponentsSymbol {
  /// Index of the component this symbol belongs to.
  pub component: usize,
  /// Index of the schematic containing this symbol.
  pub schematic: usize,
  /// Position in the schematic.
  pub position: Point,
}

/// Determine natural designators for the given components.
///
/// Symbols are grouped by their electrical connectivity, the groups are
/// ordered by schematic index and position of their top-left symbol, and the
/// symbols within each group are ordered by position. Designators are then
/// assigned in that order: Only the prefix (leading non-digit characters) and
/// suffix (characters following the first number) of each designator are
/// kept, while the number is replaced by a per-prefix counter. For example
/// `D10` might become `D1`. Components which are not added to any schematic
/// are numbered last.
///
/// # Arguments
///
/// * `components` - Current designators of all components, in circuit order.
/// * `symbols` - All symbols, ordered by schematic and their order within the
///   schematics.
/// * `connections` - Electrical connectivity as `(symbol index, net segment
///   ID)` pairs; symbols sharing a net segment ID are considered connected.
///
/// # Returns
///
/// Returns the new designators, parallel to `components`.
pub fn renumber_components(
  components: &[&str],
  symbols: &[RenumberComponentsSymbol],
  connections: &[(usize, usize)],
) -> Vec<String> {
  // Build the adjacency between symbols (through shared net segments), in
  // both directions.
  let mut nets_of_symbol: Vec<Vec<usize>> =
    symbols.iter().map(|_| Vec::new()).collect();
  let mut symbols_of_net: HashMap<usize, Vec<usize>> = HashMap::new();
  for &(symbol, net) in connections {
    nets_of_symbol[symbol].push(net);
    symbols_of_net.entry(net).or_default().push(symbol);
  }

  // Group the symbols by their electrical connectivity.
  let mut group_of_symbol = vec![usize::MAX; symbols.len()];
  let mut groups: Vec<Vec<usize>> = Vec::new();
  for start in 0..symbols.len() {
    if group_of_symbol[start] != usize::MAX {
      continue;
    }
    group_of_symbol[start] = groups.len();
    let mut group = vec![start];
    let mut i = 0;
    while i < group.len() {
      for &net in &nets_of_symbol[group[i]] {
        for &other in &symbols_of_net[&net] {
          if group_of_symbol[other] == usize::MAX {
            group_of_symbol[other] = groups.len();
            group.push(other);
          }
        }
      }
      i += 1;
    }
    groups.push(group);
  }

  // Determine the anchor of each group, i.e. its top-left position (from
  // left to right, on tie from top to bottom).
  let anchors: Vec<(Length, Length)> = groups
    .iter()
    .map(|group| {
      let left = group.iter().map(|&i| symbols[i].position.x).min().unwrap();
      let top = group
        .iter()
        .filter(|&&i| symbols[i].position.x == left)
        .map(|&i| symbols[i].position.y)
        .max()
        .unwrap();
      (left, top)
    })
    .collect();

  // Determine the order of all symbols: Groups are ordered by schematic
  // index and their anchor position, symbols within a group by their own
  // position.
  let mut order: Vec<usize> = (0..symbols.len()).collect();
  order.sort_by_cached_key(|&i| {
    let symbol = &symbols[i];
    let (left, top) = anchors[group_of_symbol[i]];
    (
      symbol.schematic,
      left,
      -top,
      symbol.position.x,
      -symbol.position.y,
    )
  });

  // Assign the new designators in that order. Components which are not
  // added to any schematic are numbered last, in circuit order.
  let mut counters: HashMap<&str, u64> = HashMap::new();
  let mut new_names: Vec<Option<String>> =
    components.iter().map(|_| None).collect();
  for i in order {
    let component = symbols[i].component;
    if new_names[component].is_none() {
      new_names[component] =
        Some(get_next_name(components[component], &mut counters));
    }
  }
  for (i, &name) in components.iter().enumerate() {
    if new_names[i].is_none() {
      new_names[i] = Some(get_next_name(name, &mut counters));
    }
  }
  new_names.into_iter().map(Option::unwrap).collect()
}

/// Get the next designator derived from the given (current) designator.
fn get_next_name<'a>(
  name: &'a str,
  counters: &mut HashMap<&'a str, u64>,
) -> String {
  let (prefix, suffix) = split_designator(name);
  let number = counters.get(prefix).copied().unwrap_or(0) + 1;
  counters.insert(prefix, number);
  format!("{}{}{}", prefix, number, suffix)
}

/// Split a designator into prefix (leading non-digit characters) and suffix
/// (everything following the first number).
///
/// This behaves like the regular expression `^([^0-9]*)([0-9]*)(.*)` with the
/// prefix and suffix as capture groups, e.g. `"ABC12XY"` -> `("ABC", "XY")`.
fn split_designator(name: &str) -> (&str, &str) {
  let prefix_end = name
    .find(|c: char| c.is_ascii_digit())
    .unwrap_or(name.len());
  let suffix_start = name[prefix_end..]
    .find(|c: char| !c.is_ascii_digit())
    .map(|i| prefix_end + i)
    .unwrap_or(name.len());
  (&name[..prefix_end], &name[suffix_start..])
}

#[cfg(test)]
mod tests {
  use super::*;
  use crate::Length;

  /// Helper to call [renumber_components] with the symbols and connections
  /// built from a list of `(component, schematic, x, y, nets)` tuples.
  fn run(
    components: &[&str],
    data: &[(usize, usize, i64, i64, &[usize])],
  ) -> Vec<String> {
    let mut symbols = Vec::new();
    let mut connections = Vec::new();
    for (i, &(component, schematic, x, y, nets)) in data.iter().enumerate() {
      symbols.push(RenumberComponentsSymbol {
        component,
        schematic,
        position: Point::new(Length::from_nm(x), Length::from_nm(y)),
      });
      connections.extend(nets.iter().map(|&net| (i, net)));
    }
    renumber_components(components, &symbols, &connections)
  }

  #[test]
  fn ordering() {
    // A project with two schematic pages containing these symbols:
    //
    //   Page 1: [R5 R10 C3] interconnected, R5 on top of R10   [U2]   [C7]
    //           x=0    x=20                                    x=50   x=80
    //           plus a second gate of C7 at x=200
    //
    //   Page 2: [R1] at x=-100 (leftmost, but on the second page)
    //
    // In addition, C1 is not added to any schematic.
    assert_eq!(
      run(
        // 0     1     2     3     4      5     6
        &["C1", "R1", "C7", "U2", "R10", "C3", "R5"],
        &[
          (6, 0, 0, 0, &[1]),   // R5
          (4, 0, 0, -10, &[1]), // R10
          (5, 0, 20, 0, &[1]),  // C3
          (3, 0, 50, 0, &[]),   // U2
          (2, 0, 80, 0, &[]),   // C7 (first gate)
          (2, 0, 200, 0, &[]),  // C7 (second gate)
          (1, 1, -100, 0, &[]), // R1
        ],
      ),
      vec!["C3", "R3", "C2", "U1", "R2", "C1", "R1"],
    );
  }

  #[test]
  fn new_names() {
    assert_eq!(
      run(&["R5A", "R6B", "C10", "LED", "7"], &[]),
      vec!["R1A", "R2B", "C1", "LED1", "1"],
    );
  }
}
