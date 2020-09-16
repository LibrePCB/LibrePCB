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

#include "cmdremoveunusedlibraryelements.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/cmd/cmdboardholeremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardplaneremove.h>
#include <librepcb/project/boards/cmd/cmdboardpolygonremove.h>
#include <librepcb/project/boards/cmd/cmdboardstroketextremove.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcb/project/boards/cmd/cmdfootprintstroketextremove.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_via.h>

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

  // also remove all netlines from devices
  foreach (BI_Device* device, mDeviceInstances) {
    Q_ASSERT(device->isAddedToBoard());
    foreach (BI_FootprintPad* pad, device->getFootprint().getPads()) {
      Q_ASSERT(pad->isAddedToBoard());
      mNetLines += pad->getNetLines();
    }
  }

  // also remove all netlines from vias
  // TODO: we shouldn't do that! but currenlty it leads to errors when not doing
  // it...
  foreach (BI_Via* via, mVias) {
    Q_ASSERT(via->isAddedToBoard());
    mNetLines += via->getNetLines();
  }

  // determine all affected netsegments and their items to remove
  NetSegmentItemList netSegmentItemsToRemove;
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
    bool removeAllVias =
        (it.value().vias == Toolbox::toSet(it.key()->getVias()));
    bool removeAllNetLines =
        (it.value().netlines == Toolbox::toSet(it.key()->getNetLines()));
    if (removeAllVias && removeAllNetLines) {
      // all items of the netsegment are selected --> remove the whole
      // netsegment
      execNewChildCmd(new CmdBoardNetSegmentRemove(*it.key()));  // can throw
    } else {
      // only some of the netsegment's lines are selected --> split up the
      // netsegment
      splitUpNetSegment(*it.key(), it.value());  // can throw
    }
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

  // remove polygons
  foreach (BI_Polygon* polygon, mPolygons) {
    Q_ASSERT(polygon->isAddedToBoard());
    execNewChildCmd(new CmdBoardPolygonRemove(*polygon));  // can throw
  }

  // remove stroke texts
  foreach (BI_StrokeText* text, mStrokeTexts) {
    if (BI_Footprint* footprint = text->getFootprint()) {
      if (!mDeviceInstances.contains(&footprint->getDeviceInstance())) {
        Q_ASSERT(text->isAddedToBoard());
        execNewChildCmd(
            new CmdFootprintStrokeTextRemove(*footprint, *text));  // can throw
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

void CmdRemoveBoardItems::splitUpNetSegment(
    BI_NetSegment& netsegment, const NetSegmentItems& itemsToRemove) {
  // determine all resulting sub-netsegments
  QVector<NetSegmentItems> subsegments =
      getNonCohesiveNetSegmentSubSegments(netsegment, itemsToRemove);

  // remove the whole netsegment
  execNewChildCmd(new CmdBoardNetSegmentRemove(netsegment));

  // create new sub-netsegments
  foreach (const NetSegmentItems& subsegment, subsegments) {
    createNewSubNetSegment(netsegment, subsegment);  // can throw
  }
}

void CmdRemoveBoardItems::createNewSubNetSegment(BI_NetSegment& netsegment,
                                                 const NetSegmentItems& items) {
  // create new netsegment
  CmdBoardNetSegmentAdd* cmdAddNetSegment = new CmdBoardNetSegmentAdd(
      netsegment.getBoard(), netsegment.getNetSignal());
  execNewChildCmd(cmdAddNetSegment);  // can throw
  BI_NetSegment* newNetSegment = cmdAddNetSegment->getNetSegment();
  Q_ASSERT(newNetSegment);
  QScopedPointer<CmdBoardNetSegmentAddElements> cmdAddElements(
      new CmdBoardNetSegmentAddElements(*newNetSegment));

  // copy vias
  QHash<const BI_NetLineAnchor*, BI_NetLineAnchor*> anchorMap;
  foreach (const BI_Via* via, items.vias) {
    BI_Via* newVia =
        cmdAddElements->addVia(Via(Uuid::createRandom(), via->getVia()));
    Q_ASSERT(newVia);
    anchorMap.insert(via, newVia);
  }

  // copy netpoints
  foreach (const BI_NetPoint* netpoint, items.netpoints) {
    BI_NetPoint* newNetPoint =
        cmdAddElements->addNetPoint(netpoint->getPosition());
    Q_ASSERT(newNetPoint);
    anchorMap.insert(netpoint, newNetPoint);
  }

  // copy netlines
  foreach (const BI_NetLine* netline, items.netlines) {
    BI_NetLineAnchor* p1 =
        anchorMap.value(&netline->getStartPoint(), &netline->getStartPoint());
    Q_ASSERT(p1);
    BI_NetLineAnchor* p2 =
        anchorMap.value(&netline->getEndPoint(), &netline->getEndPoint());
    Q_ASSERT(p2);
    BI_NetLine* newNetLine = cmdAddElements->addNetLine(
        *p1, *p2, netline->getLayer(), netline->getWidth());
    Q_ASSERT(newNetLine);
  }

  execNewChildCmd(cmdAddElements.take());  // can throw
}

QVector<CmdRemoveBoardItems::NetSegmentItems>
CmdRemoveBoardItems::getNonCohesiveNetSegmentSubSegments(
    BI_NetSegment& segment, const NetSegmentItems& removedItems) noexcept {
  // only works with segments which are added to board!!!
  Q_ASSERT(segment.isAddedToBoard());

  // get all vias and netlines of the segment to keep
  QSet<BI_Via*> vias = Toolbox::toSet(segment.getVias()) - removedItems.vias;
  QSet<BI_NetLine*> netlines =
      Toolbox::toSet(segment.getNetLines()) - removedItems.netlines;

  // find all separate segments of the netsegment
  QVector<NetSegmentItems> segments;
  while (netlines.count() + vias.count() > 0) {
    NetSegmentItems         seg;
    QSet<BI_NetLineAnchor*> processedAnchors;
    BI_NetLineAnchor*       nextAnchor =
        netlines.count() > 0 ? &netlines.values().first()->getStartPoint()
                             : vias.values().first();
    findAllConnectedNetPointsAndNetLines(*nextAnchor, processedAnchors,
                                         seg.vias, seg.netpoints, seg.netlines,
                                         vias, netlines);
    vias -= seg.vias;
    netlines -= seg.netlines;
    segments.append(seg);
  }
  Q_ASSERT(vias.isEmpty());
  Q_ASSERT(netlines.isEmpty());
  return segments;
}

void CmdRemoveBoardItems::findAllConnectedNetPointsAndNetLines(
    BI_NetLineAnchor& anchor, QSet<BI_NetLineAnchor*>& processedAnchors,
    QSet<BI_Via*>& vias, QSet<BI_NetPoint*>& netpoints,
    QSet<BI_NetLine*>& netlines, QSet<BI_Via*>& availableVias,
    QSet<BI_NetLine*>& availableNetLines) const noexcept {
  Q_ASSERT(!processedAnchors.contains(&anchor));
  processedAnchors.insert(&anchor);

  if (BI_NetPoint* anchorPoint = dynamic_cast<BI_NetPoint*>(&anchor)) {
    Q_ASSERT(!netpoints.contains(anchorPoint));
    netpoints.insert(anchorPoint);
  } else if (BI_Via* anchorVia = dynamic_cast<BI_Via*>(&anchor)) {
    Q_ASSERT(!vias.contains(anchorVia));
    vias.insert(anchorVia);
    availableVias.remove(anchorVia);
  }

  foreach (BI_NetLine* line, anchor.getNetLines()) {
    if (availableNetLines.contains(line) && (!netlines.contains(line))) {
      netlines.insert(line);
      availableNetLines.remove(line);
      BI_NetLineAnchor* p2 = line->getOtherPoint(anchor);
      Q_ASSERT(p2);
      if (!processedAnchors.contains(p2)) {
        findAllConnectedNetPointsAndNetLines(*p2, processedAnchors, vias,
                                             netpoints, netlines, availableVias,
                                             availableNetLines);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
