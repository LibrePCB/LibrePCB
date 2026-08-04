//! FFI for the orthogonal wire drag algorithm.

use crate::orthogonal_wire::*;

/// Wrapper for [AnchorKind]
#[repr(C)]
#[allow(dead_code)]
#[derive(Copy, Clone)]
enum OrthogonalWireDragAnchorKind {
  /// Anchor moved by the caller.
  Moving,
  /// Stationary anchor which may be moved by the algorithm.
  Movable,
  /// Stationary anchor which must stay fixed.
  Fixed,
}

impl From<OrthogonalWireDragAnchorKind> for AnchorKind {
  fn from(value: OrthogonalWireDragAnchorKind) -> Self {
    match value {
      OrthogonalWireDragAnchorKind::Moving => AnchorKind::Moving,
      OrthogonalWireDragAnchorKind::Movable => AnchorKind::Movable,
      OrthogonalWireDragAnchorKind::Fixed => AnchorKind::Fixed,
    }
  }
}

/// Wrapper for [Anchor]
#[repr(C)]
#[derive(Copy, Clone)]
struct OrthogonalWireDragAnchor {
  /// X coordinate in nanometers.
  x: i64,
  /// Y coordinate in nanometers.
  y: i64,
  /// Drag role.
  kind: OrthogonalWireDragAnchorKind,
}

impl From<OrthogonalWireDragAnchor> for Anchor {
  fn from(value: OrthogonalWireDragAnchor) -> Self {
    Anchor {
      x: value.x,
      y: value.y,
      kind: value.kind.into(),
    }
  }
}

/// Wrapper for [Wire]
#[repr(C)]
#[derive(Copy, Clone)]
struct OrthogonalWireDragWire {
  /// First anchor index.
  p1: usize,
  /// Second anchor index.
  p2: usize,
}

impl From<OrthogonalWireDragWire> for Wire {
  fn from(value: OrthogonalWireDragWire) -> Self {
    Wire {
      p1: value.p1,
      p2: value.p2,
    }
  }
}

/// Wrapper for [AnchorMovement]
#[repr(C)]
#[derive(Copy, Clone)]
struct OrthogonalWireDragAnchorMovement {
  /// Anchor index.
  anchor: usize,
  /// Apply the drag's X delta.
  stretch_x: bool,
  /// Apply the drag's Y delta.
  stretch_y: bool,
}

impl From<AnchorMovement> for OrthogonalWireDragAnchorMovement {
  fn from(value: AnchorMovement) -> Self {
    Self {
      anchor: value.anchor,
      stretch_x: value.mask.x,
      stretch_y: value.mask.y,
    }
  }
}

/// Wrapper for [WireSplit]
#[repr(C)]
#[derive(Copy, Clone)]
struct OrthogonalWireDragWireSplit {
  /// Wire index.
  wire: usize,
  /// Endpoint which follows the drag or propagated stretch.
  moving_anchor: usize,
  /// Endpoint which stays fixed.
  fixed_anchor: usize,
  /// Apply the drag's X delta only to decide whether this split is needed.
  trigger_x: bool,
  /// Apply the drag's Y delta only to decide whether this split is needed.
  trigger_y: bool,
  /// Apply the drag's X delta to the inserted bend point.
  ///
  /// This follows the original wire axis, not the perpendicular split trigger.
  stretch_x: bool,
  /// Apply the drag's Y delta to the inserted bend point.
  ///
  /// This follows the original wire axis, not the perpendicular split trigger.
  stretch_y: bool,
}

impl From<WireSplit> for OrthogonalWireDragWireSplit {
  fn from(value: WireSplit) -> Self {
    Self {
      wire: value.wire,
      moving_anchor: value.moving_anchor,
      fixed_anchor: value.fixed_anchor,
      trigger_x: value.trigger_mask.x,
      trigger_y: value.trigger_mask.y,
      stretch_x: value.bend_mask.x,
      stretch_y: value.bend_mask.y,
    }
  }
}

/// Wrapper for [DragResult]
struct OrthogonalWireDragOutput {
  /// Movements for existing movable anchors.
  anchor_movements: Vec<OrthogonalWireDragAnchorMovement>,
  /// Wires that need to be split into an L-bend.
  wire_splits: Vec<OrthogonalWireDragWireSplit>,
}

/// Wrapper for [calculate_orthogonal_drag]
#[no_mangle]
unsafe extern "C" fn ffi_orthogonal_wire_drag_new(
  anchors_array: *const OrthogonalWireDragAnchor,
  anchors_size: usize,
  wires_array: *const OrthogonalWireDragWire,
  wires_size: usize,
) -> *mut OrthogonalWireDragOutput {
  let anchors = std::slice::from_raw_parts(anchors_array, anchors_size)
    .iter()
    .copied()
    .map(Anchor::from)
    .collect::<Vec<_>>();
  let wires = std::slice::from_raw_parts(wires_array, wires_size)
    .iter()
    .copied()
    .map(Wire::from)
    .collect::<Vec<_>>();
  let result = calculate_orthogonal_drag(&anchors, &wires);
  Box::into_raw(Box::new(OrthogonalWireDragOutput {
    anchor_movements: result
      .anchor_movements
      .into_iter()
      .map(OrthogonalWireDragAnchorMovement::from)
      .collect(),
    wire_splits: result
      .wire_splits
      .into_iter()
      .map(OrthogonalWireDragWireSplit::from)
      .collect(),
  }))
}

/// Delete an [OrthogonalWireDragOutput]
#[no_mangle]
unsafe extern "C" fn ffi_orthogonal_wire_drag_delete(
  obj: *mut OrthogonalWireDragOutput,
) {
  if !obj.is_null() {
    drop(Box::from_raw(obj));
  }
}

/// Get number of anchor movements in an [OrthogonalWireDragOutput]
#[no_mangle]
extern "C" fn ffi_orthogonal_wire_drag_anchor_movement_count(
  obj: &OrthogonalWireDragOutput,
) -> usize {
  obj.anchor_movements.len()
}

/// Get an anchor movement from an [OrthogonalWireDragOutput]
#[no_mangle]
extern "C" fn ffi_orthogonal_wire_drag_anchor_movement(
  obj: &OrthogonalWireDragOutput,
  index: usize,
) -> OrthogonalWireDragAnchorMovement {
  obj.anchor_movements.get(index).copied().unwrap_or(
    OrthogonalWireDragAnchorMovement {
      anchor: usize::MAX,
      stretch_x: false,
      stretch_y: false,
    },
  )
}

/// Get number of wire splits in an [OrthogonalWireDragOutput]
#[no_mangle]
extern "C" fn ffi_orthogonal_wire_drag_wire_split_count(
  obj: &OrthogonalWireDragOutput,
) -> usize {
  obj.wire_splits.len()
}

/// Get a wire split from an [OrthogonalWireDragOutput]
#[no_mangle]
extern "C" fn ffi_orthogonal_wire_drag_wire_split(
  obj: &OrthogonalWireDragOutput,
  index: usize,
) -> OrthogonalWireDragWireSplit {
  obj
    .wire_splits
    .get(index)
    .copied()
    .unwrap_or(OrthogonalWireDragWireSplit {
      wire: usize::MAX,
      moving_anchor: usize::MAX,
      fixed_anchor: usize::MAX,
      trigger_x: false,
      trigger_y: false,
      stretch_x: false,
      stretch_y: false,
    })
}
