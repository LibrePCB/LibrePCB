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
#include "cmdremoveboarditems.h"

#include "../../project/cmd/cmdboardholeremove.h"
#include "../../project/cmd/cmdboardnetsegmentadd.h"
#include "../../project/cmd/cmdboardnetsegmentaddelements.h"
#include "../../project/cmd/cmdboardnetsegmentremove.h"
#include "../../project/cmd/cmdboardplaneremove.h"
#include "../../project/cmd/cmdboardpolygonremove.h"
#include "../../project/cmd/cmdboardstroketextremove.h"
#include "../../project/cmd/cmdboardzoneremove.h"
#include "../../project/cmd/cmddeviceinstanceremove.h"
#include "../../project/cmd/cmddevicestroketextremove.h"
#include "cmdremoveunusedlibraryelements.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardnetsegmentsplitter.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
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

CmdRemoveBoardItems::CmdRemoveBoardItems(Board& board) noexcept
  : UndoCommandGroup(tr("Remove Board Items")), mBoard(board) {
}

CmdRemoveBoardItems::~CmdRemoveBoardItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveBoardItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // determine all affected netsegments and their items to remove
  NetSegmentItemList netSegmentItemsToRemove;
  foreach (BI_Device* device, mDeviceInstances) {
    Q_ASSERT(device->isAddedToBoard());
    foreach (BI_FootprintPad* pad, device->getPads()) {
      if (BI_NetSegment* segment = pad->getNetSegmentOfLines()) {
        netSegmentItemsToRemove[segment].pads.insert(pad);
      }
    }
  }
  foreach (BI_Via* via, mVias) {
    Q_ASSERT(via->isAddedToBoard());
    netSegmentItemsToRemove[&via->getNetSegment()].vias.insert(via);
  }
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    Q_ASSERT(netpoint->isAddedToBoard());
    netSegmentItemsToRemove[&netpoint->getNetSegment()].netpoints.insert(
        netpoint);
  }
  foreach (BI_NetLine* netline, mNetLines) {
    Q_ASSERT(netline->isAddedToBoard());
    netSegmentItemsToRemove[&netline->getNetSegment()].netlines.insert(netline);
  }

  // remove vias/netlines/netpoints/netsegments
  for (auto it = netSegmentItemsToRemove.begin();
       it != netSegmentItemsToRemove.end(); ++it) {
    Q_ASSERT(it.key()->isAddedToBoard());
    removeNetSegmentItems(*it.key(), it.value().pads, it.value().vias,
                          it.value().netpoints,
                          it.value().netlines);  // can throw
  }

  // remove all device instances
  foreach (BI_Device* device, mDeviceInstances) {
    Q_ASSERT(device->isAddedToBoard());
    execNewChildCmd(new CmdDeviceInstanceRemove(*device));  // can throw
  }

  // remove planes
  foreach (BI_Plane* plane, mPlanes) {
    Q_ASSERT(plane->isAddedToBoard());
    execNewChildCmd(new CmdBoardPlaneRemove(*plane));  // can throw
  }

  // remove zones
  foreach (BI_Zone* zone, mZones) {
    Q_ASSERT(zone->isAddedToBoard());
    execNewChildCmd(new CmdBoardZoneRemove(*zone));  // can throw
  }

  // remove polygons
  foreach (BI_Polygon* polygon, mPolygons) {
    Q_ASSERT(polygon->isAddedToBoard());
    execNewChildCmd(new CmdBoardPolygonRemove(*polygon));  // can throw
  }

  // remove stroke texts
  foreach (BI_StrokeText* text, mStrokeTexts) {
    if (BI_Device* device = text->getDevice()) {
      if (!mDeviceInstances.contains(device)) {
        Q_ASSERT(text->isAddedToBoard());
        execNewChildCmd(
            new CmdDeviceStrokeTextRemove(*device, *text));  // can throw
      }
    } else {
      Q_ASSERT(text->isAddedToBoard());
      execNewChildCmd(new CmdBoardStrokeTextRemove(*text));  // can throw
    }
  }

  // remove holes
  foreach (BI_Hole* hole, mHoles) {
    Q_ASSERT(hole->isAddedToBoard());
    execNewChildCmd(new CmdBoardHoleRemove(*hole));  // can throw
  }

  // remove library elements which are no longer required
  if (getChildCount() > 0) {
    execNewChildCmd(
        new CmdRemoveUnusedLibraryElements(mBoard.getProject()));  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdRemoveBoardItems::removeNetSegmentItems(
    BI_NetSegment& netsegment, const QSet<BI_FootprintPad*>& padsToDisconnect,
    const QSet<BI_Via*>& viasToRemove,
    const QSet<BI_NetPoint*>& netpointsToRemove,
    const QSet<BI_NetLine*>& netlinesToRemove) {
  // Determine resulting sub-netsegments
  BoardNetSegmentSplitter splitter;
  foreach (BI_FootprintPad* pad, padsToDisconnect) {
    splitter.replaceFootprintPadByJunctions(pad->toTraceAnchor(),
                                            pad->getPosition());
  }
  foreach (BI_Via* via, netsegment.getVias()) {
    bool replaceByJunctions = viasToRemove.contains(via);
    splitter.addVia(via->getVia(), replaceByJunctions);
  }
  foreach (BI_NetPoint* netpoint, netsegment.getNetPoints()) {
    if (!netpointsToRemove.contains(netpoint)) {
      splitter.addJunction(netpoint->getJunction());
    }
  }
  foreach (BI_NetLine* netline, netsegment.getNetLines()) {
    if (!netlinesToRemove.contains(netline)) {
      splitter.addTrace(netline->getTrace());
    }
  }

  // Remove whole netsegment
  execNewChildCmd(new CmdBoardNetSegmentRemove(netsegment));  // can throw

  // Create new sub-netsegments
  foreach (const BoardNetSegmentSplitter::Segment& segment, splitter.split()) {
    // Add new netsegment
    CmdBoardNetSegmentAdd* cmdAddNetSegment = new CmdBoardNetSegmentAdd(
        netsegment.getBoard(), netsegment.getNetSignal());
    execNewChildCmd(cmdAddNetSegment);  // can throw
    BI_NetSegment* newNetSegment = cmdAddNetSegment->getNetSegment();
    Q_ASSERT(newNetSegment);

    // Add new vias, netpoints, netlines
    CmdBoardNetSegmentAddElements* cmdAddElements =
        new CmdBoardNetSegmentAddElements(*newNetSegment);
    QHash<Uuid, BI_NetLineAnchor*> viaMap;
    for (const Via& via : segment.vias) {
      BI_Via* newVia = cmdAddElements->addVia(via);
      viaMap.insert(via.getUuid(), newVia);
    }
    QHash<Uuid, BI_NetLineAnchor*> junctionMap;
    for (const Junction& junction : segment.junctions) {
      BI_NetPoint* newNetPoint =
          cmdAddElements->addNetPoint(junction.getPosition());
      junctionMap.insert(junction.getUuid(), newNetPoint);
    }
    for (const Trace& trace : segment.traces) {
      BI_NetLineAnchor* start = nullptr;
      if (tl::optional<Uuid> anchor = trace.getStartPoint().tryGetJunction()) {
        start = junctionMap[*anchor];
      } else if (tl::optional<Uuid> anchor =
                     trace.getStartPoint().tryGetVia()) {
        start = viaMap[*anchor];
      } else if (tl::optional<TraceAnchor::PadAnchor> anchor =
                     trace.getStartPoint().tryGetPad()) {
        BI_Device* device =
            mBoard.getDeviceInstanceByComponentUuid(anchor->device);
        start = device ? device->getPad(anchor->pad) : nullptr;
      }
      BI_NetLineAnchor* end = nullptr;
      if (tl::optional<Uuid> anchor = trace.getEndPoint().tryGetJunction()) {
        end = junctionMap[*anchor];
      } else if (tl::optional<Uuid> anchor = trace.getEndPoint().tryGetVia()) {
        end = viaMap[*anchor];
      } else if (tl::optional<TraceAnchor::PadAnchor> anchor =
                     trace.getEndPoint().tryGetPad()) {
        BI_Device* device =
            mBoard.getDeviceInstanceByComponentUuid(anchor->device);
        end = device ? device->getPad(anchor->pad) : nullptr;
      }
      if ((!start) || (!end)) {
        throw LogicError(__FILE__, __LINE__);
      }
      BI_NetLine* newNetLine = cmdAddElements->addNetLine(
          *start, *end, trace.getLayer(), trace.getWidth());
      Q_ASSERT(newNetLine);
    }
    execNewChildCmd(cmdAddElements);  // can throw
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
