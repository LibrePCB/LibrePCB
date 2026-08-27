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
    mTextsReset(false),
    mHasReferenceDirection(false),
    mReferenceDirection(1, 0) {
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

          // Determine the reference direction used to constrain the drag movement
          // (see documentation of #mReferenceDirection). Prefer the direction of an
          // explicitly selected/dragged trace segment; if only a lone junction
          // point is being dragged, fall back to the direction of one of its
          // connected segments so that segment keeps its angle.
  auto tryUseDirection = [&](const Point& a, const Point& b) {
    if (mHasReferenceDirection) return;
    QPointF dir = b.toMmQPointF() - a.toMmQPointF();
    const qreal len = std::hypot(dir.x(), dir.y());
    if (len > 1e-9) {
      mReferenceDirection = dir / len;
      mHasReferenceDirection = true;
    }
  };
  foreach (BI_NetLine* netline, query.getNetLines()) {
    tryUseDirection(netline->getP1().getPosition(),
                    netline->getP2().getPosition());
  }
  if (!mHasReferenceDirection) {
    foreach (BI_NetPoint* netpoint, query.getNetPoints()) {
      foreach (BI_NetLine* netline, netpoint->getNetLines()) {
        tryUseDirection(netline->getP1().getPosition(),
                        netline->getP2().getPosition());
      }
    }
  }

          // Find (at most) one neighbor trace connected to `netpoint`, other than
          // `excludeNetline`, which is *not* part of the current selection. That
          // neighbor stays where it is and must keep its own original angle - see
          // #NetPointConstraint.
  auto findNeighborRay = [&](BI_NetPoint* netpoint, BI_NetLine* excludeNetline,
                             QPointF& outFixedPoint,
                             QPointF& outDirection) -> bool {
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
        return true;
      }
    }
    return false;
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
    constraint.hasDirection = mHasReferenceDirection;
    constraint.anchorOriginalPos = constraint.originalPos;
    constraint.direction = mReferenceDirection;
    constraint.hasNeighborRay = findNeighborRay(
        netpoint, nullptr, constraint.neighborFixedPoint,
        constraint.neighborDirection);
    mNetPointConstraints.append(constraint);
  }

          // Intelligent angle-preserving trace adaptation also applies when moving
          // whole devices or vias: any trace stub connected to one of their pads
          // (resp. the via) but not itself explicitly selected is now dragged along
          // implicitly, keeping its own original angle (only its length adapts) -
          // exactly like #NetPointConstraint above, just with the pad/via (instead
          // of the point itself) as the moving anchor.
  QSet<BI_NetPoint*> cascadedNetPoints;
  auto tryCascade = [&](BI_NetLineAnchor& anchor, const Point& anchorPos) {
    foreach (BI_NetLine* netline, anchor.getNetLines()) {
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
      constraint.hasNeighborRay = findNeighborRay(
          farNetPoint, netline, constraint.neighborFixedPoint,
          constraint.neighborDirection);
      mNetPointConstraints.append(constraint);
    }
  };
  foreach (BI_Device* device, query.getDeviceInstances()) {
    foreach (BI_Pad* pad, device->getPads()) {
      tryCascade(*pad, pad->getPosition());
    }
  }
  foreach (BI_Via* via, query.getVias()) {
    tryCascade(*via, via->getPosition());
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
    return Point::fromMm(onRay);
  }
  const QPointF diff = p2 - p1;
  const qreal s = (diff.x() * d2.y() - diff.y() * d2.x()) / det;
  const QPointF intersection = p1 + s * d1;
  return Point::fromMm(intersection);
}

void CmdDragSelectedBoardItems::setCurrentPosition(
    const Point& pos, const bool gridIncrement,
    const bool freeMovement) noexcept {
  const Point rawDelta = pos - mStartPos;
  Point delta = rawDelta;

  if (mHasReferenceDirection && (!freeMovement)) {
    // Keep the reference trace (and everything moved rigidly together with
    // it) at its exact original angle: only the component of the movement
    // that is perpendicular to the reference direction is allowed. A pure
    // translation along that perpendicular can never rotate the line, so a
    // horizontal/vertical trace stays perfectly parallel to the grid axes,
    // and a trace drawn at any other angle keeps that exact angle - just
    // like dragging a track in KiCad or a wire in Eagle.
    const QPointF perp(-mReferenceDirection.y(), mReferenceDirection.x());
    const QPointF d = delta.toMmQPointF();
    const qreal t = d.x() * perp.x() + d.y() * perp.y();
    delta = Point::fromMm(t * perp.x(), t * perp.y());
  }

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