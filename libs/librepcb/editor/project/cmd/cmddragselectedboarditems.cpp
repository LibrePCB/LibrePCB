/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmddragselectedboarditems.h"

#include "../board/boardgraphicsscene.h"
#include "../board/boardselectionquery.h"
#include "../board/graphicsitems/bgi_device.h"
#include "cmdboardholeedit.h"
#include "cmdboardnetlineedit.h"
#include "cmdboardnetpointedit.h"
#include "cmdboardpadedit.h"
#include "cmdboardplaneedit.h"
#include "cmdboardpolygonedit.h"
#include "cmdboardstroketextedit.h"
#include "cmdboardviaedit.h"
#include "cmdboardzoneedit.h"
#include "cmddeviceinstanceedit.h"
#include "cmddevicestroketextsreset.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/project.h>

#include <cmath>
#include <functional>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDragSelectedBoardItems::CmdDragSelectedBoardItems(
    BoardGraphicsScene& scene, bool includeLockedItems, bool includeNetLines,
    const Point& startPos) noexcept
  : UndoCommandGroup(tr("Drag Board Elements")),
    mScene(scene),
    mItemCount(0),
    mStartPos(startPos),
    mDeltaPos(0, 0),
    mCenterPos(0, 0),
    mDeltaAngle(0),
    mSnappedToGrid(false),
    mLockedChanged(false),
    mLineWidthChanged(false),
    mTextsReset(false) {
  // get all selected items
  BoardSelectionQuery query(mScene, includeLockedItems);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedBoardPads();
  query.addSelectedVias();
  query.addSelectedNetPoints();
  if (includeNetLines) {
    query.addSelectedNetLines();
  }
  query.addSelectedNetLines();
  query.addNetPointsOfNetLines();
  query.addSelectedPlanes();
  query.addSelectedZones();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedFootprintStrokeTexts();
  query.addSelectedHoles();

          // Find (at most) one neighbor trace connected to `netpoint`, other than
          // `excludeNetline`, which is *not* part of the current selection. That
          // neighbor stays where it is and must keep its own original angle - see
          // #NetPointConstraint.
  auto findNeighborRay = [&](BI_NetPoint* netpoint, BI_NetLine* excludeNetline,
                             QPointF& outFixedPoint, QPointF& outDirection,
                             BI_NetLine** outNetline = nullptr) -> bool {
    foreach (BI_NetLine* netline, netpoint->getNetLines()) {
      if ((netline == excludeNetline) ||
          query.getNetLines().contains(netline)) {
        continue;  // Dragged along as well -> not a fixed neighbor.
      }
      BI_NetLineAnchor& farAnchor = (&netline->getP1() == netpoint)
          ? netline->getP2()
          : netline->getP1();
      QPointF dir = netpoint->getPosition().toMmQPointF() -
          farAnchor.getPosition().toMmQPointF();
      const qreal len = std::hypot(dir.x(), dir.y());
      if (len > 1e-9) {
        outFixedPoint = farAnchor.getPosition().toMmQPointF();
        outDirection = dir / len;
        if (outNetline) {
          *outNetline = netline;
        }
        return true;
      }
    }
    return false;
  };

          // Find the direction of `netpoint`'s own segment(s), i.e. the
          // segment(s) that are dragged along with it (as opposed to the
          // fixed neighbor found by findNeighborRay() above). This is what
          // "its own angle" refers to for *this* point - which, for a multi-
          // segment selection (e.g. several bends of one trace selected
          // together), is generally different from other selected points'
          // own angle. Using a single global reference direction for all of
          // them is only correct if the whole selection happens to be one
          // straight line; for a bent multi-segment selection it would
          // silently distort the selected segments while dragging. If a
          // point has no selected segment of its own at all (a lone vertex
          // grabbed directly, without Ctrl), it deliberately gets no
          // direction/constraint here - see the call site below.
  auto findOwnDirection = [&](BI_NetPoint* netpoint, BI_NetLine* excludeNetline,
                              QPointF& outDirection) -> bool {
    foreach (BI_NetLine* netline, netpoint->getNetLines()) {
      if ((netline == excludeNetline) ||
          (!query.getNetLines().contains(netline))) {
        continue;  // Not dragged along -> not "its own" segment.
      }
      BI_NetLineAnchor& farAnchor = (&netline->getP1() == netpoint)
          ? netline->getP2()
          : netline->getP1();
      QPointF dir = netpoint->getPosition().toMmQPointF() -
          farAnchor.getPosition().toMmQPointF();
      const qreal len = std::hypot(dir.x(), dir.y());
      if (len > 1e-9) {
        outDirection = dir / len;
        return true;
      }
    }
    return false;
  };

          // How many of `netpoint`'s connecting net lines are part of the
          // selection (i.e. dragged along, as opposed to fixed). This is
          // needed to tell apart a true boundary point of the selected
          // sub-path (exactly 1 selected connection - keep its own angle
          // against its 1 fixed neighbor) from a junction point where 2 or
          // more selected segments meet (must move rigidly with the group;
          // any *other*, unrelated stub at that same point must never be
          // (ab)used as a "neighbor ray" to constrain its position - doing
          // so produced wild, collapsing geometry whenever such a junction
          // was dragged).
  auto countSelectedNetLines = [&](BI_NetPoint* netpoint) -> int {
    int count = 0;
    foreach (BI_NetLine* netline, netpoint->getNetLines()) {
      if (query.getNetLines().contains(netline)) {
        ++count;
      }
    }
    return count;
  };

          // Intelligent angle-preserving trace adaptation also applies when moving
          // whole devices, vias or rigidly-moving junction points: any trace stub
          // connected to one of their pads (resp. the via, resp. the junction
          // point) but not itself explicitly selected is dragged along implicitly,
          // keeping its own original angle (only its length adapts) - exactly like
          // #NetPointConstraint above, just with the pad/via/point (instead of an
          // explicitly selected point) as the moving anchor.
  QSet<BI_NetPoint*> cascadedNetPoints;
  std::function<void(BI_NetLineAnchor&, const Point&, BI_NetLine*)>
      tryCascade;
  tryCascade = [&](BI_NetLineAnchor& anchor, const Point& anchorPos,
                   BI_NetLine* excludeNetline) {
    foreach (BI_NetLine* netline, anchor.getNetLines()) {
      if (netline == excludeNetline) {
        continue;
      }
      BI_NetLineAnchor& farAnchor = (&netline->getP1() == &anchor)
      ? netline->getP2()
      : netline->getP1();
      BI_NetPoint* farNetPoint = dynamic_cast<BI_NetPoint*>(&farAnchor);
      if ((!farNetPoint) || query.getNetPoints().contains(farNetPoint) ||
          cascadedNetPoints.contains(farNetPoint)) {
        continue;  // Not a stub, or already handled above.
      }
      QPointF dir =
          farNetPoint->getPosition().toMmQPointF() - anchorPos.toMmQPointF();
      const qreal len = std::hypot(dir.x(), dir.y());
      if (len <= 1e-9) {
        continue;  // Degenerate (coincident points).
      }
      cascadedNetPoints.insert(farNetPoint);

      CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*farNetPoint);
      mNetPointEditCmds.append(cmd);

      NetPointConstraint constraint;
      constraint.cmd = cmd;
      constraint.originalPos = farNetPoint->getPosition();
      constraint.hasDirection = true;
      constraint.anchorOriginalPos = anchorPos;
      constraint.direction = dir / len;
      BI_NetLine* neighborNetline = nullptr;
      constraint.hasNeighborRay = findNeighborRay(
          farNetPoint, netline, constraint.neighborFixedPoint,
          constraint.neighborDirection, &neighborNetline);
      mNetPointConstraints.append(constraint);
      // Recurse: this cascaded point may itself have further unselected
      // stub(s) beyond the one leading back to `anchor` and the one (if
      // any) just used as its own fixed reference ray - keep cascading
      // those too, so a whole chain of unselected trace segments follows
      // along nicely instead of only the first hop.
      tryCascade(*farNetPoint, constraint.originalPos, neighborNetline);
    }
  };

          // Expand the selected individual pads to their devices, to allow dragging
          // the devices (individual footprint pads cannot be dragged). However, the
          // devices are not selected immediately, but later in selectDevicesOfPads().
  mAutoSelectedDevices = query.addDeviceInstancesAndTextsOfSelectedPads();

          // find the center of all elements and create undo commands
  foreach (BI_Device* device, query.getDeviceInstances()) {
    Q_ASSERT(device);
    mCenterPos += device->getPosition();
    ++mItemCount;
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(*device);
    mDeviceEditCmds.append(cmd);
    mDeviceStrokeTextsResetCmds.append(new CmdDeviceStrokeTextsReset(*device));
  }
  foreach (BI_Pad* pad, query.getPads()) {
    Q_ASSERT(pad);
    mCenterPos += pad->getPosition();
    ++mItemCount;
    CmdBoardPadEdit* cmd = new CmdBoardPadEdit(*pad);
    mPadEditCmds.append(cmd);
  }
  foreach (BI_Via* via, query.getVias()) {
    Q_ASSERT(via);
    mCenterPos += via->getPosition();
    ++mItemCount;
    CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
    mViaEditCmds.append(cmd);
  }
  foreach (BI_NetPoint* netpoint, query.getNetPoints()) {
    Q_ASSERT(netpoint);
    mCenterPos += netpoint->getPosition();
    ++mItemCount;
    CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
    mNetPointEditCmds.append(cmd);

    NetPointConstraint constraint;
    constraint.cmd = cmd;
    constraint.originalPos = netpoint->getPosition();
    constraint.anchorOriginalPos = constraint.originalPos;
    constraint.hasDirection =
        findOwnDirection(netpoint, nullptr, constraint.direction);
    BI_NetLine* neighborNetline = nullptr;
    if (constraint.hasDirection && (countSelectedNetLines(netpoint) == 1)) {
      // A true boundary point of the selected sub-path: exactly one of its
      // connections is dragged along (its own segment) -> keep that
      // segment's own angle, letting one fixed neighbor adapt (if this
      // point has further unselected connections beyond that one, they are
      // cascaded below instead of being ignored/left untouched).
      constraint.hasNeighborRay = findNeighborRay(
          netpoint, nullptr, constraint.neighborFixedPoint,
          constraint.neighborDirection, &neighborNetline);
    } else {
      // Either a lone point grabbed directly by its vertex (0 selected
      // connections - plain, unconstrained point drag, all connected
      // lines freely change their angle to it, exactly like dragging a
      // bend point in KiCad), or a junction point where 2+ selected
      // segments meet (e.g. a T-junction): it moves rigidly together with
      // the rest of the selected group, preserving their combined shape -
      // it must *not* use some unrelated third/fourth stub at the same
      // point as a fake "neighbor ray" to constrain its own position (that
      // used to produce wild, collapsing geometry). All such extra
      // unselected stubs are cascaded below, just like a stub connected to
      // a dragged device pad or via.
      constraint.hasNeighborRay = false;
    }
    mNetPointConstraints.append(constraint);
    // Cascade any unselected stub(s) at this point other than the one (if
    // any) used above as the fixed reference ray - that one must stay a
    // fixed anchor, so it's excluded here.
    tryCascade(*netpoint, constraint.originalPos, neighborNetline);
  }

  foreach (BI_Device* device, query.getDeviceInstances()) {
    foreach (BI_Pad* pad, device->getPads()) {
      tryCascade(*pad, pad->getPosition(), nullptr);
    }
  }
  foreach (BI_Via* via, query.getVias()) {
    tryCascade(*via, via->getPosition(), nullptr);
  }
  foreach (BI_NetLine* netline, query.getNetLines()) {
    Q_ASSERT(netline);
    mCenterPos += netline->getP1().getPosition();
    mCenterPos += netline->getP2().getPosition();
    mItemCount += 2;
    CmdBoardNetLineEdit* cmd = new CmdBoardNetLineEdit(*netline);
    mNetLineEditCmds.append(cmd);
  }
  foreach (BI_Plane* plane, query.getPlanes()) {
    Q_ASSERT(plane);
    for (const Vertex& vertex : plane->getOutline().getVertices()) {
      mCenterPos += vertex.getPos();
      ++mItemCount;
    }
    CmdBoardPlaneEdit* cmd = new CmdBoardPlaneEdit(*plane);
    mPlaneEditCmds.append(cmd);
  }
  foreach (BI_Zone* plane, query.getZones()) {
    Q_ASSERT(plane);
    for (const Vertex& vertex : plane->getData().getOutline().getVertices()) {
      mCenterPos += vertex.getPos();
      ++mItemCount;
    }
    CmdBoardZoneEdit* cmd = new CmdBoardZoneEdit(*plane);
    mZoneEditCmds.append(cmd);
  }
  foreach (BI_Polygon* polygon, query.getPolygons()) {
    Q_ASSERT(polygon);
    for (const Vertex& vertex : polygon->getData().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      ++mItemCount;
    }
    CmdBoardPolygonEdit* cmd = new CmdBoardPolygonEdit(*polygon);
    mPolygonEditCmds.append(cmd);
  }
  foreach (BI_StrokeText* text, query.getStrokeTexts()) {
    Q_ASSERT(text);
    // do not count texts of devices if the device is selected too
    if ((!text->getDevice()) ||
        (!query.getDeviceInstances().contains(text->getDevice()))) {
      mCenterPos += text->getData().getPosition();
      ++mItemCount;
    }
    CmdBoardStrokeTextEdit* cmd = new CmdBoardStrokeTextEdit(*text);
    mStrokeTextEditCmds.append(cmd);
  }
  foreach (BI_Hole* hole, query.getHoles()) {
    Q_ASSERT(hole);
    mCenterPos += hole->getData().getPath()->getVertices().first().getPos();
    ++mItemCount;
    CmdBoardHoleEdit* cmd = new CmdBoardHoleEdit(*hole);
    mHoleEditCmds.append(cmd);
  }

          // Note: If only 1 item is selected, use its exact position as center.
  if (mItemCount > 1) {
    mCenterPos /= mItemCount;
    mCenterPos.mapToGrid(mScene.getBoard().getGridInterval());
  }
}

CmdDragSelectedBoardItems::~CmdDragSelectedBoardItems() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool CmdDragSelectedBoardItems::selectDevicesOfPads() noexcept {
  for (BI_Device* dev : std::as_const(mAutoSelectedDevices)) {
    if (auto item = mScene.getDevices().value(dev)) {
      item->setSelected(true);
    }
  }
  return !mAutoSelectedDevices.isEmpty();
}

UnsignedLength CmdDragSelectedBoardItems::getMedianLineWidth() const noexcept {
  QList<UnsignedLength> values;
  foreach (CmdBoardNetLineEdit* cmd, mNetLineEditCmds) {
    values.append(positiveToUnsigned(cmd->getObj().getWidth()));
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    values.append(cmd->getObj().getData().getLineWidth());
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    values.append(cmd->getObj().getData().getStrokeWidth());
  }
  std::sort(values.begin(), values.end());
  return values.value(values.count() / 2, UnsignedLength(0));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedBoardItems::snapToGrid() noexcept {
  PositiveLength grid = mScene.getBoard().getGridInterval();
  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardPadEdit* cmd, mPadEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardZoneEdit* cmd, mZoneEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  mSnappedToGrid = true;

          // Force updating airwires immediately as they are important while moving
          // items.
  mScene.getBoard().triggerAirWiresRebuild();
}

void CmdDragSelectedBoardItems::setLocked(bool locked) noexcept {
  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardPadEdit* cmd, mPadEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardZoneEdit* cmd, mZoneEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) {
    cmd->setLocked(locked);
  }
  mLockedChanged = true;
}

void CmdDragSelectedBoardItems::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (width > 0) {
    foreach (CmdBoardNetLineEdit* cmd, mNetLineEditCmds) {
      cmd->setWidth(PositiveLength(*width));
    }
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->setLineWidth(width, false);
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->setStrokeWidth(width, false);
  }
  mLineWidthChanged = true;
}

void CmdDragSelectedBoardItems::resetAllTexts() noexcept {
  mTextsReset = true;
}

Point CmdDragSelectedBoardItems::computeNetPointPosition(
    const NetPointConstraint& c, const Point& delta,
    const Point& rawDelta) const noexcept {
  if ((!c.hasNeighborRay) || (!c.hasDirection)) {
    // No fixed neighbor (or no direction to keep at all) -> just follow
    // the shifted line / rigid translation.
    return c.originalPos + delta;
  }

          // Never let the point slide past the fixed neighbor point along the
          // neighbor's own ray (without Ctrl held - this function is only
          // called for !freeMovement in the first place): the segment may
          // get longer or shorter, but must not cross over/past the fixed
          // point, which would otherwise fold the trace back onto/over
          // itself. Holding Ctrl bypasses this function entirely (rigid,
          // unclamped movement), so that's still how "move past a point" is
          // done intentionally.
  auto clampToNeighbor = [&](const QPointF& candidate) -> QPointF {
    const QPointF rel = candidate - c.neighborFixedPoint;
    const qreal t =
        rel.x() * c.neighborDirection.x() + rel.y() * c.neighborDirection.y();
    return (t < 0) ? c.neighborFixedPoint : candidate;
  };

          // Point on the shifted line through this point's driving anchor (the
          // point itself if it was explicitly dragged, or a moving pad/via if this
          // point is merely a stub connected to it), offset by delta, keeping its
          // own original direction.
  const QPointF p1 = c.originalPos.toMmQPointF() + delta.toMmQPointF();

          // Intersect that shifted line (p1, c.direction) with the neighbor's
          // fixed-angle ray (neighborFixedPoint, neighborDirection). Solving
          // p1 + s*d1 == p2 + t*d2 for s (Cramer's rule):
  const QPointF& d1 = c.direction;
  const QPointF& d2 = c.neighborDirection;
  const QPointF& p2 = c.neighborFixedPoint;
  const qreal det = d1.x() * d2.y() - d1.y() * d2.x();
  if (std::fabs(det) < 1e-9) {
    // Degenerate case: the neighbor's ray is parallel (or anti-parallel) to
    // this point's own direction. This happens e.g. when a trace is
    // anchored to a fixed pad on one end and you grab the whole trace:
    // there is no "other" direction to intersect with, so a naive fallback
    // to the shifted line would tilt the trace away from the pad (changing
    // its angle). Instead, only allow this point to slide *along* the
    // fixed ray (i.e. the trace may get longer/shorter but never changes
    // its angle / never tilts).
    const QPointF raw = rawDelta.toMmQPointF();
    const qreal t = raw.x() * d2.x() + raw.y() * d2.y();
    const QPointF onRay = c.originalPos.toMmQPointF() + t * d2;
    return Point::fromMm(clampToNeighbor(onRay));
  }
  const QPointF diff = p2 - p1;
  const qreal s = (diff.x() * d2.y() - diff.y() * d2.x()) / det;
  const QPointF intersection = p1 + s * d1;
  return Point::fromMm(clampToNeighbor(intersection));
}

void CmdDragSelectedBoardItems::setCurrentPosition(
    const Point& pos, const bool gridIncrement,
    const bool freeMovement) noexcept {
  const Point rawDelta = pos - mStartPos;
  Point delta = rawDelta;

  // Note: We intentionally do *not* restrict `delta` to any single
  // reference direction here (there used to be such a restriction, forcing
  // the whole selection to move only perpendicular to one segment's
  // angle - which is correct for a single straight segment, but silently
  // distorted multi-segment/bent selections). Angle preservation of each
  // selected segment is instead guaranteed individually below, per point,
  // by computeNetPointPosition(): since every selected point of the same
  // segment shares that segment's own direction and gets shifted by this
  // exact same `delta`, they necessarily stay collinear along that
  // direction no matter which way the mouse moves - so each segment keeps
  // its own angle, while only unselected/unmarked ends (the fixed
  // neighbors) adapt, exactly like in KiCad.

  if (gridIncrement) {
    delta.mapToGrid(mScene.getBoard().getGridInterval());
  }

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardPadEdit* cmd, mPadEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (const NetPointConstraint& c, mNetPointConstraints) {
      if (freeMovement) {
        // Ctrl held: bypass all angle constraints, plain rigid movement
        // like the original (pre-KiCad-style) LibrePCB behavior.
        c.cmd->setPosition(c.originalPos + delta, true);
      } else {
        c.cmd->setPosition(computeNetPointPosition(c, delta, rawDelta), true);
      }
    }
    foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardZoneEdit* cmd, mZoneEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    mDeltaPos = delta;

            // Force updating airwires immediately as they are important while moving
            // items.
    mScene.getBoard().triggerAirWiresRebuild();
  }
}

void CmdDragSelectedBoardItems::rotate(const Angle& angle,
                                       bool aroundCurrentPosition) noexcept {
  const Point center = (aroundCurrentPosition && (mItemCount > 1))
  ? (mStartPos + mDeltaPos)
          .mappedToGrid(mScene.getBoard().getGridInterval())
  : (mCenterPos + mDeltaPos);

          // rotate selected elements
  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardPadEdit* cmd, mPadEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardZoneEdit* cmd, mZoneEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) {
    cmd->rotate(angle, center, true);
  }
  mDeltaAngle += angle;

          // Force updating airwires immediately as they are important while dragging
          // items.
  mScene.getBoard().triggerAirWiresRebuild();
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedBoardItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaAngle == Angle::deg0()) &&
      (!mSnappedToGrid) && (!mTextsReset) && (!mLockedChanged) &&
      (!mLineWidthChanged)) {
    // no movement required --> discard all commands
    qDeleteAll(mDeviceEditCmds);
    mDeviceEditCmds.clear();
    qDeleteAll(mDeviceStrokeTextsResetCmds);
    mDeviceStrokeTextsResetCmds.clear();
    qDeleteAll(mPadEditCmds);
    mPadEditCmds.clear();
    qDeleteAll(mViaEditCmds);
    mViaEditCmds.clear();
    qDeleteAll(mNetPointEditCmds);
    mNetPointEditCmds.clear();
    qDeleteAll(mNetLineEditCmds);
    mNetLineEditCmds.clear();
    qDeleteAll(mPlaneEditCmds);
    mPlaneEditCmds.clear();
    qDeleteAll(mZoneEditCmds);
    mZoneEditCmds.clear();
    qDeleteAll(mPolygonEditCmds);
    mPolygonEditCmds.clear();
    qDeleteAll(mStrokeTextEditCmds);
    mStrokeTextEditCmds.clear();
    qDeleteAll(mHoleEditCmds);
    mHoleEditCmds.clear();
    return false;
  }

  if (!mTextsReset) {
    qDeleteAll(mDeviceStrokeTextsResetCmds);
    mDeviceStrokeTextsResetCmds.clear();
  }

  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdDeviceStrokeTextsReset* cmd, mDeviceStrokeTextsResetCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardPadEdit* cmd, mPadEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardNetLineEdit* cmd, mNetLineEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardZoneEdit* cmd, mZoneEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) {
    appendChild(cmd);  // can throw
  }

          // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
