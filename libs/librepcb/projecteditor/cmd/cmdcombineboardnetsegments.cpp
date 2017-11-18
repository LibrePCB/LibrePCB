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
#include "cmdcombineboardnetsegments.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include "cmdcombineboardnetpoints.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineBoardNetSegments::CmdCombineBoardNetSegments(BI_NetSegment& toBeRemoved,
                                                       BI_NetPoint& junction) noexcept :
    UndoCommandGroup(tr("Combine Board Netsegments")),
    mNetSegmentToBeRemoved(toBeRemoved), mJunctionNetPoint(junction)
{
}

CmdCombineBoardNetSegments::~CmdCombineBoardNetSegments() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineBoardNetSegments::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // check arguments validity
    if (mNetSegmentToBeRemoved == mJunctionNetPoint.getNetSegment())
        throw LogicError(__FILE__, __LINE__);
    if (mNetSegmentToBeRemoved.getNetSignal() != mJunctionNetPoint.getNetSignalOfNetSegment())
        throw LogicError(__FILE__, __LINE__);

    // find all interception netpoints
    QList<BI_NetPoint*> netpointsUnderJunction;
    mNetSegmentToBeRemoved.getNetPointsAtScenePos(mJunctionNetPoint.getPosition(),
        &mJunctionNetPoint.getLayer(), netpointsUnderJunction);

    // create exactly one interception netpoint
    BI_NetPoint* interceptionNetPoint = nullptr;
    if (netpointsUnderJunction.count() == 0) {
        QList<BI_NetLine*> netlinesUnderJunction;
        mNetSegmentToBeRemoved.getNetLinesAtScenePos(mJunctionNetPoint.getPosition(),
            &mJunctionNetPoint.getLayer(), netlinesUnderJunction);
        if (netlinesUnderJunction.count() == 0) {
            QList<BI_Via*> viasUnderJunction;
            mNetSegmentToBeRemoved.getViasAtScenePos(mJunctionNetPoint.getPosition(),
                                                     viasUnderJunction);
            if (viasUnderJunction.count() == 1) {
                interceptionNetPoint = &addNetPointToVia(*viasUnderJunction.first());
            } else {
                throw RuntimeError(__FILE__, __LINE__, tr("Sorry, not yet implemented..."));
            }
        } else {
            Q_ASSERT(netlinesUnderJunction.count() > 0);
            BI_NetLine* netline = netlinesUnderJunction.first(); Q_ASSERT(netline);
            interceptionNetPoint = &addNetPointInMiddleOfNetLine(*netline, mJunctionNetPoint.getPosition());
        }
    } else if (netpointsUnderJunction.count() == 1) {
        interceptionNetPoint = netpointsUnderJunction.first();
    } else {
        Q_ASSERT(netpointsUnderJunction.count() > 1);
        interceptionNetPoint = netpointsUnderJunction.first();
        foreach (BI_NetPoint* netpoint, netpointsUnderJunction) {
            if (netpoint != interceptionNetPoint) {
                execNewChildCmd(new CmdCombineBoardNetPoints(
                                    *netpoint, *interceptionNetPoint)); // can throw
            }
        }
    }
    Q_ASSERT(interceptionNetPoint);
    Q_ASSERT(&interceptionNetPoint->getNetSegment() == &mNetSegmentToBeRemoved);

    // move all required vias/netpoints/netlines to the resulting netsegment
    CmdBoardNetSegmentAddElements* cmdAdd = new CmdBoardNetSegmentAddElements(mJunctionNetPoint.getNetSegment());
    QHash<BI_Via*, BI_Via*> viaMap;
    foreach (BI_Via* via, mNetSegmentToBeRemoved.getVias()) {
        BI_Via* newVia = cmdAdd->addVia(via->getPosition(), via->getShape(), via->getSize(), via->getDrillDiameter());
        viaMap.insert(via, newVia);
    }
    QHash<BI_NetPoint*, BI_NetPoint*> netPointMap;
    foreach (BI_NetPoint* netpoint, mNetSegmentToBeRemoved.getNetPoints()) {
        if (netpoint == interceptionNetPoint) {
            netPointMap.insert(netpoint, &mJunctionNetPoint);
        } else if (netpoint->isAttachedToPad()) {
            BI_FootprintPad* pad = netpoint->getFootprintPad(); Q_ASSERT(pad);
            BI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getLayer(), *pad); Q_ASSERT(newNetPoint);
            netPointMap.insert(netpoint, newNetPoint);
        } else if (netpoint->isAttachedToVia()) {
            BI_Via* via = viaMap.value(netpoint->getVia()); Q_ASSERT(via);
            BI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getLayer(), *via); Q_ASSERT(newNetPoint);
            netPointMap.insert(netpoint, newNetPoint);
        } else {
            BI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getLayer(), netpoint->getPosition()); Q_ASSERT(newNetPoint);
            netPointMap.insert(netpoint, newNetPoint);
        }
    }
    foreach (BI_NetLine* netline, mNetSegmentToBeRemoved.getNetLines()) {
        BI_NetPoint* startPoint = netPointMap.value(&netline->getStartPoint()); Q_ASSERT(startPoint);
        BI_NetPoint* endPoint = netPointMap.value(&netline->getEndPoint()); Q_ASSERT(endPoint);
        BI_NetLine* newNetLine = cmdAdd->addNetLine(*startPoint, *endPoint, netline->getWidth()); Q_ASSERT(newNetLine);
    }
    execNewChildCmd(new CmdBoardNetSegmentRemove(mNetSegmentToBeRemoved)); // can throw
    execNewChildCmd(cmdAdd); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BI_NetPoint& CmdCombineBoardNetSegments::addNetPointToVia(BI_Via& via)
{
    CmdBoardNetSegmentAddElements* cmdAdd = new CmdBoardNetSegmentAddElements(via.getNetSegment());
    BI_NetPoint* netpoint = cmdAdd->addNetPoint(mJunctionNetPoint.getLayer(), via); Q_ASSERT(netpoint);
    execNewChildCmd(cmdAdd); // can throw
    return *netpoint;
}

BI_NetPoint& CmdCombineBoardNetSegments::addNetPointInMiddleOfNetLine(BI_NetLine& l, const Point& pos)
{
    // add netpoint + 2 netlines
    CmdBoardNetSegmentAddElements* cmdAdd = new CmdBoardNetSegmentAddElements(l.getNetSegment());
    BI_NetPoint* netpoint = cmdAdd->addNetPoint(l.getLayer(), pos); Q_ASSERT(netpoint);
    BI_NetLine* netline1 = cmdAdd->addNetLine(l.getStartPoint(), *netpoint, l.getWidth()); Q_ASSERT(netline1);
    BI_NetLine* netline2 = cmdAdd->addNetLine(l.getEndPoint(), *netpoint, l.getWidth()); Q_ASSERT(netline2);
    execNewChildCmd(cmdAdd); // can throw

    // remove netline
    CmdBoardNetSegmentRemoveElements* cmdRemove = new CmdBoardNetSegmentRemoveElements(l.getNetSegment());
    cmdRemove->removeNetLine(l);
    execNewChildCmd(cmdRemove); // can throw

    return *netpoint;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
