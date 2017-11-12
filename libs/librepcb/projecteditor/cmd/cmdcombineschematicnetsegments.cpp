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
#include "cmdcombineschematicnetsegments.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include "cmdremoveunusednetsignals.h"
#include "cmdcombineschematicnetpoints.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineSchematicNetSegments::CmdCombineSchematicNetSegments(SI_NetSegment& toBeRemoved,
                                                               SI_NetPoint& junction) noexcept :
    UndoCommandGroup(tr("Combine Schematic Netsegments")),
    mNetSegmentToBeRemoved(toBeRemoved), mJunctionNetPoint(junction)
{
}

CmdCombineSchematicNetSegments::~CmdCombineSchematicNetSegments() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineSchematicNetSegments::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // check arguments validity
    if (mNetSegmentToBeRemoved == mJunctionNetPoint.getNetSegment())
        throw LogicError(__FILE__, __LINE__);
    if (mNetSegmentToBeRemoved.getNetSignal() != mJunctionNetPoint.getNetSignalOfNetSegment())
        throw LogicError(__FILE__, __LINE__);

    // find all interception netpoints
    QList<SI_NetPoint*> netpointsUnderJunction;
    mNetSegmentToBeRemoved.getNetPointsAtScenePos(mJunctionNetPoint.getPosition(), netpointsUnderJunction);

    // create exactly one interception netpoint
    SI_NetPoint* interceptionNetPoint = nullptr;
    if (netpointsUnderJunction.count() == 0) {
        QList<SI_NetLine*> netlinesUnderJunction;
        mNetSegmentToBeRemoved.getNetLinesAtScenePos(mJunctionNetPoint.getPosition(), netlinesUnderJunction);
        if (netlinesUnderJunction.count() == 0) {
            throw LogicError(__FILE__, __LINE__);
        } else {
            Q_ASSERT(netlinesUnderJunction.count() > 0);
            SI_NetLine* netline = netlinesUnderJunction.first(); Q_ASSERT(netline);
            interceptionNetPoint = &addNetPointInMiddleOfNetLine(*netline, mJunctionNetPoint.getPosition());
        }
    } else if (netpointsUnderJunction.count() == 1) {
        interceptionNetPoint = netpointsUnderJunction.first();
    } else {
        Q_ASSERT(netpointsUnderJunction.count() > 1);
        interceptionNetPoint = netpointsUnderJunction.first();
        foreach (SI_NetPoint* netpoint, netpointsUnderJunction) {
            if (netpoint != interceptionNetPoint) {
                execNewChildCmd(new CmdCombineSchematicNetPoints(
                                    *netpoint, *interceptionNetPoint)); // can throw
            }
        }
    }
    Q_ASSERT(interceptionNetPoint);

    // move all required netpoints/netlines to the resulting netsegment
    CmdSchematicNetSegmentAddElements* cmdAdd = new CmdSchematicNetSegmentAddElements(mJunctionNetPoint.getNetSegment());
    QHash<SI_NetPoint*, SI_NetPoint*> netPointMap;
    foreach (SI_NetPoint* netpoint, mNetSegmentToBeRemoved.getNetPoints()) {
        if (netpoint == interceptionNetPoint) {
            netPointMap.insert(netpoint, &mJunctionNetPoint);
        } else if (netpoint->isAttachedToPin()) {
            SI_SymbolPin* pin = netpoint->getSymbolPin(); Q_ASSERT(pin);
            SI_NetPoint* newNetPoint = cmdAdd->addNetPoint(*pin); Q_ASSERT(newNetPoint);
            netPointMap.insert(netpoint, newNetPoint);
        } else {
            SI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getPosition()); Q_ASSERT(newNetPoint);
            netPointMap.insert(netpoint, newNetPoint);
        }
    }
    foreach (SI_NetLine* netline, mNetSegmentToBeRemoved.getNetLines()) {
        SI_NetPoint* startPoint = netPointMap.value(&netline->getStartPoint()); Q_ASSERT(startPoint);
        SI_NetPoint* endPoint = netPointMap.value(&netline->getEndPoint()); Q_ASSERT(endPoint);
        SI_NetLine* newNetLine = cmdAdd->addNetLine(*startPoint, *endPoint); Q_ASSERT(newNetLine);
    }
    execNewChildCmd(new CmdSchematicNetSegmentRemove(mNetSegmentToBeRemoved)); // can throw
    execNewChildCmd(cmdAdd); // can throw


    if (getChildCount() > 0) {
        // remove netsignals which are no longer required
        execNewChildCmd(new CmdRemoveUnusedNetSignals(mJunctionNetPoint.getCircuit())); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SI_NetPoint& CmdCombineSchematicNetSegments::addNetPointInMiddleOfNetLine(
        SI_NetLine& l, const Point& pos)
{
    // add netpoint + 2 netlines
    CmdSchematicNetSegmentAddElements* cmdAdd = new CmdSchematicNetSegmentAddElements(l.getNetSegment());
    SI_NetPoint* netpoint = cmdAdd->addNetPoint(pos); Q_ASSERT(netpoint);
    SI_NetLine* netline1 = cmdAdd->addNetLine(l.getStartPoint(), *netpoint); Q_ASSERT(netline1);
    SI_NetLine* netline2 = cmdAdd->addNetLine(l.getEndPoint(), *netpoint); Q_ASSERT(netline2);
    execNewChildCmd(cmdAdd); // can throw

    // remove netline
    CmdSchematicNetSegmentRemoveElements* cmdRemove = new CmdSchematicNetSegmentRemoveElements(l.getNetSegment());
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
