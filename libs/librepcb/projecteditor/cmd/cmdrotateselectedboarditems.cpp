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
#include "cmdrotateselectedboarditems.h"

#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/polygon.h>
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
#include <librepcb/project/boards/items/bi_plane.h>
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

CmdRotateSelectedBoardItems::CmdRotateSelectedBoardItems(
    Board& board, const Angle& angle) noexcept
  : UndoCommandGroup(tr("Rotate Board Elements")),
    mBoard(board),
    mAngle(angle) {
}

CmdRotateSelectedBoardItems::~CmdRotateSelectedBoardItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRotateSelectedBoardItems::performExecute() {
  // get all selected items
  std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
  query->addDeviceInstancesOfSelectedFootprints();
  query->addSelectedVias();
  query->addSelectedNetPoints();
  query->addNetPointsOfNetLines();
  query->addSelectedPlanes();
  query->addSelectedPolygons();
  query->addSelectedBoardStrokeTexts();
  query->addSelectedFootprintStrokeTexts();
  query->addSelectedHoles();

  // find the center of all elements
  Point center = Point(0, 0);
  int   count  = 0;
  foreach (BI_Device* device, query->getDeviceInstances()) {
    center += device->getPosition();
    ++count;
  }
  foreach (BI_Via* via, query->getVias()) {
    center += via->getPosition();
    ++count;
  }
  foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
    center += netpoint->getPosition();
    ++count;
  }
  foreach (BI_Plane* plane, query->getPlanes()) {
    for (const Vertex& vertex : plane->getOutline().getVertices()) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    for (const Vertex& vertex : polygon->getPolygon().getPath().getVertices()) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (BI_StrokeText* text, query->getStrokeTexts()) {
    // do not count texts of footprints if the footprint is selected too
    if ((!text->getFootprint()) ||
        (!query->getDeviceInstances().contains(
            &text->getFootprint()->getDeviceInstance()))) {
      center += text->getPosition();
      ++count;
    }
  }
  foreach (BI_Hole* hole, query->getHoles()) {
    center += hole->getPosition();
    ++count;
  }

  if (count > 0) {
    center /= count;
    center.mapToGrid(mBoard.getGridProperties().getInterval());
  } else {
    // no items selected --> nothing to do here
    return false;
  }

  // rotate all selected elements
  foreach (BI_Device* device, query->getDeviceInstances()) {
    Q_ASSERT(device);
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(*device);
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (BI_Via* via, query->getVias()) {
    Q_ASSERT(via);
    CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
    cmd->setPosition(via->getPosition().rotated(mAngle, center), false);
    appendChild(cmd);
  }
  foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
    Q_ASSERT(netpoint);
    CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
    cmd->setPosition(netpoint->getPosition().rotated(mAngle, center), false);
    appendChild(cmd);
  }
  foreach (BI_Plane* plane, query->getPlanes()) {
    Q_ASSERT(plane);
    CmdBoardPlaneEdit* cmd = new CmdBoardPlaneEdit(*plane, false);
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    Q_ASSERT(polygon);
    CmdPolygonEdit* cmd = new CmdPolygonEdit(polygon->getPolygon());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (BI_StrokeText* text, query->getStrokeTexts()) {
    Q_ASSERT(text);
    CmdStrokeTextEdit* cmd = new CmdStrokeTextEdit(text->getText());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (BI_Hole* hole, query->getHoles()) {
    Q_ASSERT(hole);
    CmdHoleEdit* cmd = new CmdHoleEdit(hole->getHole());
    cmd->setPosition(hole->getPosition().rotated(mAngle, center), false);
    appendChild(cmd);
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
