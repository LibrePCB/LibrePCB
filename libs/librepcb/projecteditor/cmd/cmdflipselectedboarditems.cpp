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
#include "cmdflipselectedboarditems.h"

#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/scopeguard.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netline.h>
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

CmdFlipSelectedBoardItems::CmdFlipSelectedBoardItems(
    Board& board, Qt::Orientation orientation) noexcept
  : UndoCommandGroup(tr("Flip Board Elements")),
    mBoard(board),
    mOrientation(orientation) {
}

CmdFlipSelectedBoardItems::~CmdFlipSelectedBoardItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFlipSelectedBoardItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // get all selected items
  std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
  query->addDeviceInstancesOfSelectedFootprints();
  query->addSelectedNetLines();
  query->addSelectedVias();
  query->addSelectedPlanes();
  query->addSelectedPolygons();
  query->addSelectedBoardStrokeTexts();
  query->addSelectedFootprintStrokeTexts();
  query->addSelectedHoles();
  query->addNetPointsOfNetLines();

  // find the center of all elements
  Point center = Point(0, 0);
  int   count  = 0;
  foreach (BI_Device* device, query->getDeviceInstances()) {
    center += device->getPosition();
    ++count;
  }
  foreach (BI_NetLine* netline, query->getNetLines()) {
    center += netline->getPosition();
    ++count;
  }
  foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
    center += netpoint->getPosition();
    ++count;
  }
  foreach (BI_Via* via, query->getVias()) {
    center += via->getPosition();
    ++count;
  }
  foreach (BI_Plane* plane, query->getPlanes()) {
    for (const Vertex& vertex :
         plane->getOutline().getVertices().toList().toSet()) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    for (const Vertex& vertex :
         polygon->getPolygon().getPath().getVertices().toList().toSet()) {
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
  } else {
    // no items selected --> nothing to do here
    undoScopeGuard.dismiss();
    return false;
  }

  // TODO: make this feature more sophisticated!

  // determine affected netsegments
  QSet<BI_NetSegment*> netsegments;
  foreach (BI_NetLine* netline, query->getNetLines()) {
    netsegments.insert(&netline->getNetSegment());
  }
  foreach (BI_Via* via, query->getVias()) {
    netsegments.insert(&via->getNetSegment());
  }
  foreach (BI_Device* device, query->getDeviceInstances()) {
    foreach (BI_FootprintPad* pad, device->getFootprint().getPads()) {
      if (pad->getNetSegmentOfLines()) {
        netsegments.insert(pad->getNetSegmentOfLines());
      }
    }
  }

  // temporary disconnect netsegments (pads and netlines can only be mirrored if
  // they are unconnected)
  foreach (BI_NetSegment* netsegment, netsegments) {
    execNewChildCmd(new CmdBoardNetSegmentRemove(*netsegment));  // can throw
  }

  // flip all device instances
  foreach (BI_Device* device, query->getDeviceInstances()) {
    Q_ASSERT(device);
    QScopedPointer<CmdDeviceInstanceEdit> cmd(
        new CmdDeviceInstanceEdit(*device));
    cmd->mirror(center, mOrientation, false);  // can throw
    execNewChildCmd(cmd.take());               // can throw
  }

  // mirror all netlines
  foreach (BI_NetLine* netline, query->getNetLines()) {
    Q_ASSERT(netline);
    GraphicsLayer* layer = mBoard.getLayerStack().getLayer(
        GraphicsLayer::getMirroredLayerName(netline->getLayer().getName()));
    Q_ASSERT(layer);
    QScopedPointer<CmdBoardNetLineEdit> cmd(new CmdBoardNetLineEdit(*netline));
    cmd->setLayer(*layer);
    execNewChildCmd(cmd.take());  // can throw
  }

  // move all netpoints
  foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
    Q_ASSERT(netpoint);
    QScopedPointer<CmdBoardNetPointEdit> cmd(
        new CmdBoardNetPointEdit(*netpoint));
    cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center),
                     false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // move all vias
  foreach (BI_Via* via, query->getVias()) {
    Q_ASSERT(via);
    QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(*via));
    cmd->setPosition(via->getPosition().mirrored(mOrientation, center), false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all planes
  foreach (BI_Plane* plane, query->getPlanes()) {
    QScopedPointer<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(*plane, false));
    cmd->mirror(center, mOrientation, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all polygons
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    QScopedPointer<CmdPolygonEdit> cmd(
        new CmdPolygonEdit(polygon->getPolygon()));
    cmd->mirror(mOrientation, center, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all stroke texts
  foreach (BI_StrokeText* text, query->getStrokeTexts()) {
    QScopedPointer<CmdStrokeTextEdit> cmd(
        new CmdStrokeTextEdit(text->getText()));
    cmd->mirror(mOrientation, center, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // move all holes
  foreach (BI_Hole* hole, query->getHoles()) {
    QScopedPointer<CmdHoleEdit> cmd(new CmdHoleEdit(hole->getHole()));
    cmd->setPosition(hole->getPosition().mirrored(mOrientation, center), false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // reconnect all netsegments
  foreach (BI_NetSegment* netsegment, netsegments) {
    execNewChildCmd(new CmdBoardNetSegmentAdd(*netsegment));  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
