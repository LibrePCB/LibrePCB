//! Orthogonal wire drag calculation.

use std::collections::HashMap;

/// The role of a wire anchor during a drag operation.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub(crate) enum AnchorKind {
  /// The anchor is moved by the caller's normal drag operation.
  Moving,
  /// The anchor is stationary, but the algorithm may move it to keep wires
  /// orthogonal.
  Movable,
  /// The anchor must stay in place. A boundary wire to this anchor may need an
  /// inserted bend point.
  Fixed,
}

/// A point-like endpoint in the wire graph.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub(crate) struct Anchor {
  /// X coordinate in nanometers.
  pub x: i64,
  /// Y coordinate in nanometers.
  pub y: i64,
  /// Drag role.
  pub kind: AnchorKind,
}

/// A wire between two anchors.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub(crate) struct Wire {
  /// First anchor index.
  pub p1: usize,
  /// Second anchor index.
  pub p2: usize,
}

/// Axis mask of a propagated movement.
#[derive(Debug, Copy, Clone, Default, PartialEq, Eq)]
pub(crate) struct AxisMask {
  /// Apply the drag's X delta.
  pub x: bool,
  /// Apply the drag's Y delta.
  pub y: bool,
}

impl AxisMask {
  /// Create a new mask.
  fn new(x: bool, y: bool) -> Self {
    Self { x, y }
  }

  /// Check whether no axis is set.
  fn is_empty(&self) -> bool {
    !self.x && !self.y
  }

  /// Merge another mask into this one.
  ///
  /// Returns `true` if the mask gained at least one new axis.
  fn merge(&mut self, other: Self) -> bool {
    let before = *self;
    self.x |= other.x;
    self.y |= other.y;
    *self != before
  }
}

/// Movement to apply to a movable anchor.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub(crate) struct AnchorMovement {
  /// Anchor index.
  pub anchor: usize,
  /// Axes to move.
  pub mask: AxisMask,
}

/// Wire which needs a bend point inserted at its moving side.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub(crate) struct WireSplit {
  /// Wire index.
  pub wire: usize,
  /// Endpoint which follows the drag or propagated stretch.
  pub moving_anchor: usize,
  /// Endpoint which stays fixed.
  pub fixed_anchor: usize,
  /// Axes of the drag which make this split necessary.
  pub trigger_mask: AxisMask,
  /// Axes to move the inserted bend point.
  ///
  /// The bend follows the moving endpoint only along the original wire axis, so
  /// the untouched side of the wire can stay in place.
  pub bend_mask: AxisMask,
}

/// Result of an orthogonal wire drag calculation.
#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct DragResult {
  /// Movements for existing movable anchors.
  pub anchor_movements: Vec<AnchorMovement>,
  /// Wires that need to be split into an L-bend.
  pub wire_splits: Vec<WireSplit>,
}

/// Mutable state used while calculating anchor movements and boundary splits.
struct CalculationState<'a> {
  /// Input anchors.
  anchors: &'a [Anchor],
  /// Accumulated movement masks per movable anchor.
  anchor_masks: HashMap<usize, AxisMask>,
  /// Accumulated split masks per boundary wire.
  wire_split_masks: HashMap<usize, WireSplit>,
}

impl<'a> CalculationState<'a> {
  /// Create empty state for an anchor set.
  fn new(anchors: &'a [Anchor]) -> Self {
    Self {
      anchors,
      anchor_masks: HashMap::new(),
      wire_split_masks: HashMap::new(),
    }
  }

  /// Accumulate movement for one existing movable anchor.
  fn add_anchor_movement(&mut self, anchor: usize, mask: AxisMask) {
    if mask.is_empty() {
      return;
    }
    if self.anchors[anchor].kind != AnchorKind::Movable {
      return;
    }
    self.anchor_masks.entry(anchor).or_default().merge(mask);
  }

  /// Add a boundary split between one endpoint which follows the drag and one
  /// endpoint which stays at its original position.
  fn add_wire_split(
    &mut self,
    wire_index: usize,
    moving_anchor: usize,
    fixed_anchor: usize,
    trigger_mask: AxisMask,
    bend_mask: AxisMask,
  ) {
    if trigger_mask.is_empty() {
      return;
    }
    let split = self
      .wire_split_masks
      .entry(wire_index)
      .or_insert(WireSplit {
        wire: wire_index,
        moving_anchor,
        fixed_anchor,
        trigger_mask: AxisMask::default(),
        bend_mask: AxisMask::default(),
      });
    split.moving_anchor = moving_anchor;
    split.fixed_anchor = fixed_anchor;
    split.trigger_mask.merge(trigger_mask);
    split.bend_mask.merge(bend_mask);
  }

  /// Movement mask for one anchor. Directly dragged anchors follow both axes;
  /// movable anchors follow only axes discovered from adjacent moving anchors.
  fn movement_mask(&self, anchor: usize) -> AxisMask {
    match self.anchors[anchor].kind {
      AnchorKind::Moving => AxisMask::new(true, true),
      AnchorKind::Movable => {
        self.anchor_masks.get(&anchor).copied().unwrap_or_default()
      }
      AnchorKind::Fixed => AxisMask::default(),
    }
  }
}

/// Calculate how stationary anchors need to move to keep wires orthogonal.
///
/// The algorithm starts at anchors marked as [`AnchorKind::Moving`]. If a
/// directly connected wire was horizontal before the drag, the neighbour follows
/// the drag's Y delta; if it was vertical, the neighbour follows the X delta.
/// The movement is intentionally limited to that one neighbouring vertex. If an
/// adjacent wire would otherwise become diagonal, the caller can split it when
/// the trigger mask is active and move the inserted bend point with the returned
/// bend mask.
pub(crate) fn calculate_orthogonal_drag(
  anchors: &[Anchor],
  wires: &[Wire],
) -> DragResult {
  let adjacency = build_adjacency(anchors, wires);
  let mut state = CalculationState::new(anchors);

  for anchor_index in moving_anchors(anchors) {
    for (_, other_index) in &adjacency[anchor_index] {
      let mask = initial_mask(anchors[anchor_index], anchors[*other_index]);
      state.add_anchor_movement(*other_index, mask);
    }
  }

  for (wire_index, wire) in wires.iter().enumerate() {
    if wire.p1 >= anchors.len() || wire.p2 >= anchors.len() {
      continue;
    }
    if let Some((moving_anchor, fixed_anchor, trigger_mask, bend_mask)) =
      split_for_different_endpoint_movement(wire, &state)
    {
      state.add_wire_split(
        wire_index,
        moving_anchor,
        fixed_anchor,
        trigger_mask,
        bend_mask,
      );
    }
  }

  let mut anchor_movements = state
    .anchor_masks
    .into_iter()
    .filter(|(_, mask)| !mask.is_empty())
    .map(|(anchor, mask)| AnchorMovement { anchor, mask })
    .collect::<Vec<_>>();
  anchor_movements.sort_by_key(|movement| movement.anchor);

  let mut wire_splits =
    state.wire_split_masks.into_values().collect::<Vec<_>>();
  wire_splits.sort_by_key(|split| split.wire);

  DragResult {
    anchor_movements,
    wire_splits,
  }
}

/// Build anchor-to-wire adjacency and ignore wires with invalid endpoints.
fn build_adjacency(
  anchors: &[Anchor],
  wires: &[Wire],
) -> Vec<Vec<(usize, usize)>> {
  let mut adjacency = vec![Vec::new(); anchors.len()];
  for (wire_index, wire) in wires.iter().enumerate() {
    if wire.p1 >= anchors.len() || wire.p2 >= anchors.len() {
      continue;
    }
    adjacency[wire.p1].push((wire_index, wire.p2));
    adjacency[wire.p2].push((wire_index, wire.p1));
  }
  adjacency
}

/// Return all anchors moved directly by the caller.
fn moving_anchors(anchors: &[Anchor]) -> impl Iterator<Item = usize> + '_ {
  anchors
    .iter()
    .enumerate()
    .filter(|(_, anchor)| anchor.kind == AnchorKind::Moving)
    .map(|(index, _)| index)
}

/// Determine the first stretch mask from a moving anchor to its neighbour.
fn initial_mask(moving: Anchor, other: Anchor) -> AxisMask {
  AxisMask::new(moving.x == other.x, moving.y == other.y)
}

/// Determine whether a wire needs a split because only one endpoint moves
/// perpendicular to its original orientation.
fn split_for_different_endpoint_movement(
  wire: &Wire,
  state: &CalculationState,
) -> Option<(usize, usize, AxisMask, AxisMask)> {
  let p1 = state.anchors[wire.p1];
  let p2 = state.anchors[wire.p2];
  let p1_mask = state.movement_mask(wire.p1);
  let p2_mask = state.movement_mask(wire.p2);

  if p1.x == p2.x {
    split_for_axis(
      wire.p1,
      p1_mask,
      wire.p2,
      p2_mask,
      AxisMask::new(true, false),
      AxisMask::new(false, true),
    )
  } else if p1.y == p2.y {
    split_for_axis(
      wire.p1,
      p1_mask,
      wire.p2,
      p2_mask,
      AxisMask::new(false, true),
      AxisMask::new(true, false),
    )
  } else {
    None
  }
}

/// Build a split when exactly one endpoint moves along the wire's perpendicular
/// axis. If both endpoints move, or neither does, the wire stays orthogonal as-is.
fn split_for_axis(
  p1: usize,
  p1_mask: AxisMask,
  p2: usize,
  p2_mask: AxisMask,
  trigger_axis: AxisMask,
  wire_axis: AxisMask,
) -> Option<(usize, usize, AxisMask, AxisMask)> {
  match (
    mask_contains(p1_mask, trigger_axis),
    mask_contains(p2_mask, trigger_axis),
  ) {
    (true, false) => {
      Some((p1, p2, trigger_axis, mask_intersection(p1_mask, wire_axis)))
    }
    (false, true) => {
      Some((p2, p1, trigger_axis, mask_intersection(p2_mask, wire_axis)))
    }
    _ => None,
  }
}

/// Check whether all axes from `needle` are set in `mask`.
fn mask_contains(mask: AxisMask, needle: AxisMask) -> bool {
  (!needle.x || mask.x) && (!needle.y || mask.y)
}

/// Keep only axes set in both masks.
fn mask_intersection(a: AxisMask, b: AxisMask) -> AxisMask {
  AxisMask::new(a.x && b.x, a.y && b.y)
}

#[cfg(test)]
mod tests {
  use super::*;

  fn moving(x: i64, y: i64) -> Anchor {
    Anchor {
      x,
      y,
      kind: AnchorKind::Moving,
    }
  }

  fn movable(x: i64, y: i64) -> Anchor {
    Anchor {
      x,
      y,
      kind: AnchorKind::Movable,
    }
  }

  fn fixed(x: i64, y: i64) -> Anchor {
    Anchor {
      x,
      y,
      kind: AnchorKind::Fixed,
    }
  }

  fn wire(p1: usize, p2: usize) -> Wire {
    Wire { p1, p2 }
  }

  fn axes(x: bool, y: bool) -> AxisMask {
    AxisMask::new(x, y)
  }

  #[test]
  fn direct_pin_to_pin_wire_creates_split() {
    let result =
      calculate_orthogonal_drag(&[moving(0, 0), fixed(10, 0)], &[wire(0, 1)]);

    assert_eq!(result.anchor_movements, vec![]);
    assert_eq!(
      result.wire_splits,
      vec![WireSplit {
        wire: 0,
        moving_anchor: 0,
        fixed_anchor: 1,
        trigger_mask: axes(false, true),
        bend_mask: axes(true, false),
      }]
    );
  }

  #[test]
  fn direct_pin_to_netpoint_moves_netpoint() {
    let result =
      calculate_orthogonal_drag(&[moving(0, 0), movable(10, 0)], &[wire(0, 1)]);

    assert_eq!(
      result.anchor_movements,
      vec![AnchorMovement {
        anchor: 1,
        mask: axes(false, true),
      }]
    );
    assert_eq!(result.wire_splits, vec![]);
  }

  #[test]
  fn direct_netpoint_move_splits_next_fixed_boundary() {
    let result = calculate_orthogonal_drag(
      &[moving(0, 0), movable(10, 0), fixed(20, 0)],
      &[wire(0, 1), wire(1, 2)],
    );

    assert_eq!(
      result.anchor_movements,
      vec![AnchorMovement {
        anchor: 1,
        mask: axes(false, true),
      }]
    );
    assert_eq!(
      result.wire_splits,
      vec![WireSplit {
        wire: 1,
        moving_anchor: 1,
        fixed_anchor: 2,
        trigger_mask: axes(false, true),
        bend_mask: axes(false, false),
      }]
    );
  }

  #[test]
  fn movement_stops_after_one_movable_anchor() {
    let result = calculate_orthogonal_drag(
      &[moving(0, 0), movable(10, 0), movable(20, 0), fixed(30, 0)],
      &[wire(0, 1), wire(1, 2), wire(2, 3)],
    );

    assert_eq!(
      result.anchor_movements,
      vec![AnchorMovement {
        anchor: 1,
        mask: axes(false, true),
      }]
    );
    assert_eq!(
      result.wire_splits,
      vec![WireSplit {
        wire: 1,
        moving_anchor: 1,
        fixed_anchor: 2,
        trigger_mask: axes(false, true),
        bend_mask: axes(false, false),
      }]
    );
  }

  #[test]
  fn same_netpoint_can_receive_both_axes() {
    let result = calculate_orthogonal_drag(
      &[moving(0, 10), moving(10, 0), movable(10, 10), fixed(20, 10)],
      &[wire(0, 2), wire(1, 2), wire(2, 3)],
    );

    assert_eq!(
      result.anchor_movements,
      vec![AnchorMovement {
        anchor: 2,
        mask: axes(true, true),
      }]
    );
    assert_eq!(
      result.wire_splits,
      vec![WireSplit {
        wire: 2,
        moving_anchor: 2,
        fixed_anchor: 3,
        trigger_mask: axes(false, true),
        bend_mask: axes(true, false),
      }]
    );
  }

  #[test]
  fn vertical_wire_split_moves_bend_only_along_wire_axis() {
    let result = calculate_orthogonal_drag(
      &[moving(0, 0), movable(0, 10), fixed(0, 20)],
      &[wire(0, 1), wire(1, 2)],
    );

    assert_eq!(
      result.anchor_movements,
      vec![AnchorMovement {
        anchor: 1,
        mask: axes(true, false),
      }]
    );
    assert_eq!(
      result.wire_splits,
      vec![WireSplit {
        wire: 1,
        moving_anchor: 1,
        fixed_anchor: 2,
        trigger_mask: axes(true, false),
        bend_mask: axes(false, false),
      }]
    );
  }

  #[test]
  fn diagonal_wires_are_left_unchanged() {
    let result = calculate_orthogonal_drag(
      &[moving(0, 0), movable(10, 10)],
      &[wire(0, 1)],
    );

    assert_eq!(result.anchor_movements, vec![]);
    assert_eq!(result.wire_splits, vec![]);
  }

  #[test]
  fn moving_anchor_at_other_end_is_ignored() {
    let result =
      calculate_orthogonal_drag(&[moving(0, 0), moving(10, 0)], &[wire(0, 1)]);

    assert_eq!(result.anchor_movements, vec![]);
    assert_eq!(result.wire_splits, vec![]);
  }
}
