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

#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDragSelectedBoardItems::CmdDragSelectedBoardItems(
    Board& board, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Drag Board Elements")),
    mBoard(board),
    mStartPos(startPos),
    mDeltaPos(0, 0),
    mCenterPos(0, 0),
    mDeltaAngle(0) {
  // get all selected items
  std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
  query->addDeviceInstancesOfSelectedFootprints();
  query->addSelectedVias();
  query->addSelectedNetPoints();
  query->addSelectedNetLines();
  query->addNetPointsOfNetLines();
  query->addSelectedPlanes();
  query->addSelectedPolygons();
  query->addSelectedBoardStrokeTexts();
  query->addSelectedFootprintStrokeTexts();
  query->addSelectedHoles();

  // find the center of all elements and create undo commands
  int count = 0;
  foreach (BI_Device* device, query->getDeviceInstances()) {
    Q_ASSERT(device);
    mCenterPos += device->getPosition();
    ++count;
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(*device);
    mDeviceEditCmds.append(cmd);
  }
  foreach (BI_Via* via, query->getVias()) {
    Q_ASSERT(via);
    mCenterPos += via->getPosition();
    ++count;
    CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
    mViaEditCmds.append(cmd);
  }
  foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
    Q_ASSERT(netpoint);
    mCenterPos += netpoint->getPosition();
    ++count;
    CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
    mNetPointEditCmds.append(cmd);
  }
  foreach (BI_Plane* plane, query->getPlanes()) {
    Q_ASSERT(plane);
    for (const Vertex& vertex : plane->getOutline().getVertices()) {
      mCenterPos += vertex.getPos();
      ++count;
    }
    CmdBoardPlaneEdit* cmd = new CmdBoardPlaneEdit(*plane, false);
    mPlaneEditCmds.append(cmd);
  }
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    Q_ASSERT(polygon);
    for (const Vertex& vertex : polygon->getPolygon().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      ++count;
    }
    CmdPolygonEdit* cmd = new CmdPolygonEdit(polygon->getPolygon());
    mPolygonEditCmds.append(cmd);
  }
  foreach (BI_StrokeText* text, query->getStrokeTexts()) {
    Q_ASSERT(text);
    // do not count texts of footprints if the footprint is selected too
    if ((!text->getFootprint()) ||
        (!query->getDeviceInstances().contains(
            &text->getFootprint()->getDeviceInstance()))) {
      mCenterPos += text->getPosition();
      ++count;
    }
    CmdStrokeTextEdit* cmd = new CmdStrokeTextEdit(text->getText());
    mStrokeTextEditCmds.append(cmd);
  }
  foreach (BI_Hole* hole, query->getHoles()) {
    Q_ASSERT(hole);
    mCenterPos += hole->getPosition();
    ++count;
    CmdHoleEdit* cmd = new CmdHoleEdit(hole->getHole());
    mHoleEditCmds.append(cmd);
  }

  if (count > 0) {
    mCenterPos /= count;
    mCenterPos.mapToGrid(mBoard.getGridProperties().getInterval());
  }
}

CmdDragSelectedBoardItems::~CmdDragSelectedBoardItems() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedBoardItems::setCurrentPosition(
    const Point& pos, const bool gridIncrement) noexcept {
  Point delta = pos - mStartPos;
  if (gridIncrement) {
    delta.mapToGrid(mBoard.getGridProperties().getInterval());
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
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdStrokeTextEdit* cmd, mStrokeTextEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    mDeltaPos = delta;

    // Force updating airwires immediately as they are important while moving
    // items.
    mBoard.triggerAirWiresRebuild();
  }
}

void CmdDragSelectedBoardItems::rotate(const Angle& angle,
                                       bool aroundItemsCenter) noexcept {
  Point center = (aroundItemsCenter ? mCenterPos : mStartPos) + mDeltaPos;

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
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->rotate(angle, center, true);
  }
  mDeltaAngle += angle;

  // Force updating airwires immediately as they are important while dragging
  // items.
  mBoard.triggerAirWiresRebuild();
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedBoardItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaAngle == Angle::deg0())) {
    // no movement required --> discard all commands
    qDeleteAll(mDeviceEditCmds);
    mDeviceEditCmds.clear();
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

  foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
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
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdStrokeTextEdit* cmd, mStrokeTextEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    appendChild(cmd);  // can throw
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
