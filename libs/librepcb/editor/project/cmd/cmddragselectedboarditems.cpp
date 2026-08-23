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

#include <QtCore>

#include <cmath>

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
  }
  foreach (BI_NetLine* netline, query.getNetLines()) {
    Q_ASSERT(netline);
    mCenterPos += netline->getP1().getPosition();
    mCenterPos += netline->getP2().getPosition();
    mItemCount += 2;
    CmdBoardNetLineEdit* cmd = new CmdBoardNetLineEdit(*netline);
    mNetLineEditCmds.append(cmd);
  }

          // Intelligent angle-preserving trace adaptation: when a whole
          // device (or a via) is dragged, any trace stub connected to one of
          // its pads (resp. the via) but not itself explicitly selected is
          // dragged along implicitly too, keeping its own original angle
          // (only its length adapts) - instead of just being left behind
          // with a now-wrong, kinked angle. Holding Ctrl while dragging (see
          // #setCurrentPosition()'s freeMovement parameter) disables this
          // and moves the device with all stubs following it rigidly,
          // exactly like before this feature existed.
          //
          // Deliberately limited to a *single* hop (the one net point
          // directly touching the pad/via): walking arbitrarily long chains
          // of bend points sounds appealing, but on a densely routed real
          // board it can chain through many segments far away from the part
          // being dragged, and if two of those distant segments happen to
          // be near-parallel, the angle-intersection math places the point
          // at a huge, effectively unbounded distance - sending traces
          // flying across the whole board. Only adapting the immediate stub
          // segment is what a component move should affect anyway.
  auto computeRayTo = [](BI_NetLineAnchor& point, BI_NetLine& line,
                         QPointF& outFixedPoint,
                         QPointF& outDirection) -> bool {
    BI_NetLineAnchor& farAnchor =
        (&line.getP1() == &point) ? line.getP2() : line.getP1();
    QPointF dir = point.getPosition().toMmQPointF() -
        farAnchor.getPosition().toMmQPointF();
    const qreal len = std::hypot(dir.x(), dir.y());
    if (len <= 1e-9) {
      return false;
    }
    outFixedPoint = farAnchor.getPosition().toMmQPointF();
    outDirection = dir / len;
    return true;
  };
          // True if `anchor` (a pad or via) is part of the current drag
          // itself, in which case it must never be treated as a fixed
          // neighbor - its position already follows the drag automatically.
  auto isMovingPadOrVia = [&](BI_NetLineAnchor& anchor) -> bool {
    if (BI_Pad* pad = dynamic_cast<BI_Pad*>(&anchor)) {
      return pad->getDevice() &&
          query.getDeviceInstances().contains(pad->getDevice());
    }
    if (BI_Via* via = dynamic_cast<BI_Via*>(&anchor)) {
      return query.getVias().contains(via);
    }
    return false;
  };
  QSet<BI_NetPoint*> processedFarNetPoints;
  auto tryCascade = [&](BI_NetLineAnchor& anchor, const Point& anchorPos) {
    foreach (BI_NetLine* netline, anchor.getNetLines()) {
      BI_NetLineAnchor& farAnchor = (&netline->getP1() == &anchor)
          ? netline->getP2()
          : netline->getP1();
      BI_NetPoint* farNetPoint = dynamic_cast<BI_NetPoint*>(&farAnchor);
      if ((!farNetPoint) || query.getNetPoints().contains(farNetPoint) ||
          processedFarNetPoints.contains(farNetPoint)) {
        continue;  // Not a stub, already handled directly above, or already
                  // reached from a different pad/via of the same drag.
      }
      QPointF dir =
          farNetPoint->getPosition().toMmQPointF() - anchorPos.toMmQPointF();
      const qreal len = std::hypot(dir.x(), dir.y());
      if (len <= 1e-9) {
        continue;  // Degenerate (coincident points).
      }
      processedFarNetPoints.insert(farNetPoint);

              // Look at every *other* line connected to this point (besides
              // the one leading back to the moved pad/via) to find a
              // genuinely fixed neighbor to keep at its exact original angle
              // - but only if there's exactly one such neighbor. With 2+ (a
              // real junction with several independently fixed branches),
              // there is no single angle that can keep all of them exact,
              // and guessing which one to intersect against would silently
              // distort a branch that has nothing to do with the current
              // drag. In that ambiguous case, this point is left completely
              // untouched instead: only the segment to the dragged pad/via
              // changes (stretches/rotates freely to follow it), while
              // everything else around the junction stays perfectly still -
              // this is also how KiCad-style footprint dragging behaves at
              // real junctions.
      int fixedNeighborCount = 0;
      QPointF fixedNeighborPoint, fixedNeighborDir;
      foreach (BI_NetLine* nl, farNetPoint->getNetLines()) {
        if (nl == netline) {
          continue;
        }
        BI_NetLineAnchor& otherEnd =
            (&nl->getP1() == farNetPoint) ? nl->getP2() : nl->getP1();
        if (isMovingPadOrVia(otherEnd)) {
          continue;  // Follows the drag automatically - not a constraint.
        }
        QPointF p, d;
        if (computeRayTo(*farNetPoint, *nl, p, d)) {
          if (fixedNeighborCount == 0) {
            fixedNeighborPoint = p;
            fixedNeighborDir = d;
          }
          ++fixedNeighborCount;
        }
      }
      if (fixedNeighborCount >= 2) {
        continue;  // Real junction with 2+ fixed branches - leave it alone.
      }

      CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*farNetPoint);
      mNetPointEditCmds.append(cmd);

      NetPointConstraint constraint;
      constraint.cmd = cmd;
      constraint.originalPos = farNetPoint->getPosition();
      constraint.hasDirection = true;
      constraint.anchorOriginalPos = anchorPos;
      constraint.direction = dir / len;
      constraint.hasNeighborRay = (fixedNeighborCount == 1);
      if (constraint.hasNeighborRay) {
        constraint.neighborFixedPoint = fixedNeighborPoint;
        constraint.neighborDirection = fixedNeighborDir;
      }
      mNetPointConstraints.append(constraint);
      mConstrainedNetPointCmds.insert(cmd);
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

void CmdDragSelectedBoardItems::setCurrentPosition(
    const Point& pos, const bool gridIncrement,
    const bool freeMovement) noexcept {
  Point delta = pos - mStartPos;
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
      const Point newPos = freeMovement
          ? (c.originalPos + delta)
          : computeNetPointPosition(c, delta);
      c.cmd->setPosition(newPos, true);
    }
    foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
      if (!mConstrainedNetPointCmds.contains(cmd)) {
        cmd->translate(delta - mDeltaPos, true);
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

Point CmdDragSelectedBoardItems::computeNetPointPosition(
    const NetPointConstraint& c, const Point& delta) const noexcept {
  if ((!c.hasNeighborRay) || (!c.hasDirection)) {
    // No fixed neighbor to intersect with (a dangling stub end, or a real
    // junction that's intentionally left untouched elsewhere) - just
    // translate rigidly by the same delta as the driving anchor. For a
    // dangling end this exactly preserves both the original angle and
    // length (the whole stub moves as one rigid piece with the pad/via).
    return c.originalPos + delta;
  }
  const QPointF p1 = c.anchorOriginalPos.toMmQPointF() + delta.toMmQPointF();
  const QPointF& d1 = c.direction;
  const QPointF& d2 = c.neighborDirection;
  const QPointF& p2 = c.neighborFixedPoint;
  const qreal det = d1.x() * d2.y() - d1.y() * d2.x();
  if (std::fabs(det) < 1e-9) {
    // The two segments are (numerically) parallel - no well-defined
    // intersection exists; fall back to a plain rigid translation rather
    // than dividing by ~0 and producing a point far off the board.
    return c.originalPos + delta;
  }
  const QPointF diff = p2 - p1;
  const qreal s = (diff.x() * d2.y() - diff.y() * d2.x()) / det;
  const QPointF intersection = p1 + s * d1;
  return Point::fromMm(intersection);
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
