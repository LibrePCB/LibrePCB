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

#include "../boardeditor/boardgraphicsscene.h"
#include "../boardeditor/boardselectionquery.h"
#include "cmdboardholeedit.h"
#include "cmdboardnetpointedit.h"
#include "cmdboardplaneedit.h"
#include "cmdboardpolygonedit.h"
#include "cmdboardstroketextedit.h"
#include "cmdboardviaedit.h"
#include "cmddeviceinstanceedit.h"
#include "cmddevicestroketextsreset.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/project.h>

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
    BoardGraphicsScene& scene, bool includeLockedItems,
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
    mTextsReset(false) {
  // get all selected items
  BoardSelectionQuery query(mScene, includeLockedItems);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedVias();
  query.addSelectedNetPoints();
  query.addSelectedNetLines();
  query.addNetPointsOfNetLines();
  query.addSelectedPlanes();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedFootprintStrokeTexts();
  query.addSelectedHoles();

  // find the center of all elements and create undo commands
  foreach (BI_Device* device, query.getDeviceInstances()) {
    Q_ASSERT(device);
    mCenterPos += device->getPosition();
    ++mItemCount;
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(*device);
    mDeviceEditCmds.append(cmd);
    mDeviceStrokeTextsResetCmds.append(new CmdDeviceStrokeTextsReset(*device));
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
  foreach (BI_Plane* plane, query.getPlanes()) {
    Q_ASSERT(plane);
    for (const Vertex& vertex : plane->getOutline().getVertices()) {
      mCenterPos += vertex.getPos();
      ++mItemCount;
    }
    CmdBoardPlaneEdit* cmd = new CmdBoardPlaneEdit(*plane);
    mPlaneEditCmds.append(cmd);
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
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedBoardItems::snapToGrid() noexcept {
  PositiveLength grid = mScene.getBoard().getGridInterval();
  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) { cmd->snapToGrid(grid, true); }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
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
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) { cmd->setLocked(locked); }
  foreach (CmdBoardPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->setLocked(locked);
  }
  foreach (CmdBoardHoleEdit* cmd, mHoleEditCmds) { cmd->setLocked(locked); }
  mLockedChanged = true;
}

void CmdDragSelectedBoardItems::resetAllTexts() noexcept {
  mTextsReset = true;
}

void CmdDragSelectedBoardItems::setCurrentPosition(
    const Point& pos, const bool gridIncrement) noexcept {
  Point delta = pos - mStartPos;
  if (gridIncrement) {
    delta.mapToGrid(mScene.getBoard().getGridInterval());
  }

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
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
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
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
      (!mSnappedToGrid) && (!mTextsReset) && (!mLockedChanged)) {
    // no movement required --> discard all commands
    qDeleteAll(mDeviceEditCmds);
    mDeviceEditCmds.clear();
    qDeleteAll(mDeviceStrokeTextsResetCmds);
    mDeviceStrokeTextsResetCmds.clear();
    qDeleteAll(mViaEditCmds);
    mViaEditCmds.clear();
    qDeleteAll(mNetPointEditCmds);
    mNetPointEditCmds.clear();
    qDeleteAll(mPlaneEditCmds);
    mPlaneEditCmds.clear();
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
  foreach (CmdBoardViaEdit* cmd, mViaEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardNetPointEdit* cmd, mNetPointEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdBoardPlaneEdit* cmd, mPlaneEditCmds) {
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
