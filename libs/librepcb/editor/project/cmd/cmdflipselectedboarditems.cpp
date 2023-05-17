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

#include "../boardeditor/boardgraphicsscene.h"
#include "../boardeditor/boardselectionquery.h"
#include "cmdboardholeedit.h"
#include "cmdboardnetlineedit.h"
#include "cmdboardnetpointedit.h"
#include "cmdboardnetsegmentadd.h"
#include "cmdboardnetsegmentremove.h"
#include "cmdboardplaneedit.h"
#include "cmdboardpolygonedit.h"
#include "cmdboardstroketextedit.h"
#include "cmdboardviaedit.h"
#include "cmddeviceinstanceedit.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFlipSelectedBoardItems::CmdFlipSelectedBoardItems(
    BoardGraphicsScene& scene, Qt::Orientation orientation,
    bool includeLockedItems) noexcept
  : UndoCommandGroup(tr("Flip Board Elements")),
    mScene(scene),
    mOrientation(orientation),
    mIncludeLockedItems(includeLockedItems) {
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
  BoardSelectionQuery query(mScene, mIncludeLockedItems);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedNetLines();
  query.addSelectedVias();
  query.addSelectedPlanes();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedFootprintStrokeTexts();
  query.addSelectedHoles();
  query.addNetPointsOfNetLines();

  // find the center of all elements
  Point center = Point(0, 0);
  int count = 0;
  foreach (BI_Device* device, query.getDeviceInstances()) {
    center += device->getPosition();
    ++count;
  }
  foreach (BI_NetLine* netline, query.getNetLines()) {
    center += netline->getStartPoint().getPosition();
    center += netline->getEndPoint().getPosition();
    count += 2;
  }
  foreach (BI_NetPoint* netpoint, query.getNetPoints()) {
    center += netpoint->getPosition();
    ++count;
  }
  foreach (BI_Via* via, query.getVias()) {
    center += via->getPosition();
    ++count;
  }
  foreach (BI_Plane* plane, query.getPlanes()) {
    for (const Vertex& vertex :
         Toolbox::toSet(plane->getOutline().getVertices().toList())) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (BI_Polygon* polygon, query.getPolygons()) {
    for (const Vertex& vertex :
         Toolbox::toSet(polygon->getData().getPath().getVertices().toList())) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (BI_StrokeText* text, query.getStrokeTexts()) {
    // do not count texts of devices if the device is selected too
    if ((!text->getDevice()) ||
        (!query.getDeviceInstances().contains(text->getDevice()))) {
      center += text->getData().getPosition();
      ++count;
    }
  }
  foreach (BI_Hole* hole, query.getHoles()) {
    center += hole->getData().getPath()->getVertices().first().getPos();
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
  foreach (BI_NetLine* netline, query.getNetLines()) {
    netsegments.insert(&netline->getNetSegment());
  }
  foreach (BI_Via* via, query.getVias()) {
    netsegments.insert(&via->getNetSegment());
  }
  foreach (BI_Device* device, query.getDeviceInstances()) {
    foreach (BI_FootprintPad* pad, device->getPads()) {
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
  foreach (BI_Device* device, query.getDeviceInstances()) {
    Q_ASSERT(device);
    QScopedPointer<CmdDeviceInstanceEdit> cmd(
        new CmdDeviceInstanceEdit(*device));
    cmd->mirror(center, mOrientation, false);  // can throw
    execNewChildCmd(cmd.take());  // can throw
  }

  // mirror all netlines
  const int innerLayerCount = mScene.getBoard().getInnerLayerCount();
  foreach (BI_NetLine* netline, query.getNetLines()) {
    Q_ASSERT(netline);
    QScopedPointer<CmdBoardNetLineEdit> cmd(new CmdBoardNetLineEdit(*netline));
    cmd->setLayer(netline->getLayer().mirrored(innerLayerCount));
    execNewChildCmd(cmd.take());  // can throw
  }

  // move all netpoints
  foreach (BI_NetPoint* netpoint, query.getNetPoints()) {
    Q_ASSERT(netpoint);
    QScopedPointer<CmdBoardNetPointEdit> cmd(
        new CmdBoardNetPointEdit(*netpoint));
    cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center),
                     false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // move all vias
  foreach (BI_Via* via, query.getVias()) {
    Q_ASSERT(via);
    QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(*via));
    cmd->setPosition(via->getPosition().mirrored(mOrientation, center), false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all planes
  foreach (BI_Plane* plane, query.getPlanes()) {
    QScopedPointer<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(*plane));
    cmd->mirror(center, mOrientation, innerLayerCount, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all polygons
  foreach (BI_Polygon* polygon, query.getPolygons()) {
    QScopedPointer<CmdBoardPolygonEdit> cmd(new CmdBoardPolygonEdit(*polygon));
    cmd->mirrorGeometry(mOrientation, center, false);
    cmd->mirrorLayer(innerLayerCount, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // flip all stroke texts
  foreach (BI_StrokeText* text, query.getStrokeTexts()) {
    QScopedPointer<CmdBoardStrokeTextEdit> cmd(
        new CmdBoardStrokeTextEdit(*text));
    cmd->mirrorGeometry(mOrientation, center, false);
    cmd->mirrorLayer(innerLayerCount, false);
    execNewChildCmd(cmd.take());  // can throw
  }

  // mirror all holes
  foreach (BI_Hole* hole, query.getHoles()) {
    QScopedPointer<CmdBoardHoleEdit> cmd(new CmdBoardHoleEdit(*hole));
    cmd->mirror(mOrientation, center, false);
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
}  // namespace librepcb
