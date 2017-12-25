/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdremoveselectedboarditems.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardpolygonremove.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include "cmdremovedevicefromboard.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRemoveSelectedBoardItems::CmdRemoveSelectedBoardItems(Board& board) noexcept :
    UndoCommandGroup(tr("Remove Board Elements")), mBoard(board)
{
}

CmdRemoveSelectedBoardItems::~CmdRemoveSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRemoveSelectedBoardItems::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
    query->addSelectedFootprints();
    query->addSelectedVias();
    query->addSelectedNetLines(BoardSelectionQuery::NetLineFilter::All);
    query->addNetPointsOfNetLines(BoardSelectionQuery::NetLineFilter::All,
                                  BoardSelectionQuery::NetPointFilter::AllConnectedLinesSelected);
    query->addSelectedPolygons();

    // clear selection because these items will be removed now
    mBoard.clearSelection();

    // determine all affected netsegments and their items
    NetSegmentItemList netSegmentItems;
    foreach (BI_Via* via, query->getVias()) {
        NetSegmentItems& items = netSegmentItems[&via->getNetSegment()];
        items.vias.insert(via);
    }
    foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
        NetSegmentItems& items = netSegmentItems[&netpoint->getNetSegment()];
        items.netpoints.insert(netpoint);
    }
    foreach (BI_NetLine* netline, query->getNetLines()) {
        NetSegmentItems& items = netSegmentItems[&netline->getNetSegment()];
        items.netlines.insert(netline);
    }

    // remove vias/netlines/netpoints/netsegments
    foreach (BI_NetSegment* netsegment, netSegmentItems.keys()) {
        const NetSegmentItems& items = netSegmentItems.value(netsegment);
        bool removeAllVias = (items.vias == netsegment->getVias().toSet());
        bool removeAllNetLines = (items.netlines == netsegment->getNetLines().toSet());
        if (removeAllVias && removeAllNetLines) {
            // all lines of the netsegment are selected --> remove the whole netsegment
            execNewChildCmd(new CmdBoardNetSegmentRemove(*netsegment)); // can throw
        } else {
            // only some of the netsegment's lines are selected --> split up the netsegment
            splitUpNetSegment(*netsegment, items); // can throw
        }
    }

    // remove all device instances
    foreach (BI_Footprint* footprint, query->getFootprints()) {
        BI_Device& device = footprint->getDeviceInstance();
        execNewChildCmd(new CmdRemoveDeviceFromBoard(device)); // can throw
    }

    // remove polygons
    foreach (BI_Polygon* polygon, query->getPolygons()) {
        execNewChildCmd(new CmdBoardPolygonRemove(*polygon)); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

void CmdRemoveSelectedBoardItems::splitUpNetSegment(BI_NetSegment& netsegment,
                                                    const NetSegmentItems& selectedItems)
{
    // determine all resulting sub-netsegments
    QList<NetSegmentItems> subsegments = getNonCohesiveNetSegmentSubSegments(netsegment,
                                                                             selectedItems);

    // remove the whole netsegment
    execNewChildCmd(new CmdBoardNetSegmentRemove(netsegment));

    // create new sub-netsegments
    foreach (const NetSegmentItems& subsegment, subsegments) {
        createNewSubNetSegment(netsegment, subsegment); // can throw
    }
}

void CmdRemoveSelectedBoardItems::createNewSubNetSegment(BI_NetSegment& netsegment,
                                                         const NetSegmentItems& items)
{
    // create new netsegment
    CmdBoardNetSegmentAdd* cmdAddNetSegment = new CmdBoardNetSegmentAdd(
        netsegment.getBoard(), netsegment.getNetSignal());
    execNewChildCmd(cmdAddNetSegment); // can throw
    BI_NetSegment* newNetSegment = cmdAddNetSegment->getNetSegment(); Q_ASSERT(newNetSegment);
    CmdBoardNetSegmentAddElements* cmdAddElements = new CmdBoardNetSegmentAddElements(*newNetSegment);

    // copy vias
    QHash<const BI_Via*, BI_Via*> viaMap;
    foreach (const BI_Via* via, items.vias) {
        BI_Via* newVia = cmdAddElements->addVia(via->getPosition(), via->getShape(),
                                                via->getSize(), via->getDrillDiameter());
        Q_ASSERT(newVia);
        viaMap.insert(via, newVia);
    }

    // copy netpoints
    QHash<const BI_NetPoint*, BI_NetPoint*> netPointMap;
    foreach (const BI_NetPoint* netpoint, items.netpoints) {
        BI_NetPoint* newNetPoint;
        if (netpoint->isAttachedToPad()) {
            BI_FootprintPad* pad = netpoint->getFootprintPad(); Q_ASSERT(pad);
            newNetPoint = cmdAddElements->addNetPoint(netpoint->getLayer(), *pad);
        } else if (BI_Via* via = viaMap.value(netpoint->getVia())) {
            newNetPoint = cmdAddElements->addNetPoint(netpoint->getLayer(), *via);
        } else {
            newNetPoint = cmdAddElements->addNetPoint(netpoint->getLayer(), netpoint->getPosition());
        }
        Q_ASSERT(newNetPoint);
        netPointMap.insert(netpoint, newNetPoint);
    }

    // copy netlines
    foreach (const BI_NetLine* netline, items.netlines) {
        BI_NetPoint* p1 = netPointMap.value(&netline->getStartPoint()); Q_ASSERT(p1);
        BI_NetPoint* p2 = netPointMap.value(&netline->getEndPoint()); Q_ASSERT(p2);
        BI_NetLine* newNetLine = cmdAddElements->addNetLine(*p1, *p2, netline->getWidth());
        Q_ASSERT(newNetLine);
    }

    execNewChildCmd(cmdAddElements); // can throw
}

QList<CmdRemoveSelectedBoardItems::NetSegmentItems>
CmdRemoveSelectedBoardItems::getNonCohesiveNetSegmentSubSegments(
        BI_NetSegment& segment, const NetSegmentItems& removedItems) noexcept
{
    // get all vias, netpoints and netlines of the segment to keep
    QSet<BI_Via*> vias = segment.getVias().toSet() - removedItems.vias;
    QSet<BI_NetPoint*> netpoints = segment.getNetPoints().toSet() - removedItems.netpoints;
    QSet<BI_NetLine*> netlines = segment.getNetLines().toSet() - removedItems.netlines;

    // find all separate segments of the netsegment
    QList<NetSegmentItems> segments;
    while (netpoints.count() > 0) {
        NetSegmentItems seg;
        findAllConnectedNetPointsAndNetLines(*netpoints.values().first(),
                                             vias, netpoints, netlines,
                                             seg.vias, seg.netpoints, seg.netlines);
        foreach (BI_Via* v, seg.vias) vias.remove(v);
        foreach (BI_NetPoint* p, seg.netpoints) netpoints.remove(p);
        foreach (BI_NetLine* l, seg.netlines) netlines.remove(l);
        segments.append(seg);
    }
    foreach (BI_Via* via, vias) {
        NetSegmentItems seg;
        seg.vias.insert(via);
        segments.append(seg);
        vias.remove(via);
    }
    Q_ASSERT(vias.isEmpty());
    Q_ASSERT(netpoints.isEmpty());
    Q_ASSERT(netlines.isEmpty());
    return segments;
}

void CmdRemoveSelectedBoardItems::findAllConnectedNetPointsAndNetLines(
        BI_NetPoint& netpoint, QSet<BI_Via*>& availableVias,
        QSet<BI_NetPoint*>& availableNetPoints, QSet<BI_NetLine*>& availableNetLines,
        QSet<BI_Via*>& vias, QSet<BI_NetPoint*>& netpoints, QSet<BI_NetLine*>& netlines) const noexcept
{
    Q_ASSERT(!netpoints.contains(&netpoint));
    Q_ASSERT(availableNetPoints.contains(&netpoint));
    netpoints.insert(&netpoint);
    if (netpoint.isAttachedToVia()) {
        BI_Via* via = netpoint.getVia(); Q_ASSERT(via);
        if ((availableVias.contains(via)) && (!vias.contains(via))) {
            vias.insert(via);
            foreach (BI_NetPoint* np, via->getNetPoints()) {
                if ((availableNetPoints.contains(np)) && (!netpoints.contains(np))) {
                    findAllConnectedNetPointsAndNetLines(*np, availableVias,
                        availableNetPoints, availableNetLines, vias, netpoints, netlines);
                }
            }
        }
    }
    foreach (BI_NetLine* line, netpoint.getLines()) {
        if (availableNetLines.contains(line)) {
            netlines.insert(line);
            BI_NetPoint* p2 = line->getOtherPoint(netpoint); Q_ASSERT(p2);
            if ((availableNetPoints.contains(p2)) && (!netpoints.contains(p2))) {
                findAllConnectedNetPointsAndNetLines(*p2, availableVias,
                    availableNetPoints, availableNetLines, vias, netpoints, netlines);
            }
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
