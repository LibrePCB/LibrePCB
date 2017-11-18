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
#include "cmdcombineallitemsunderboardnetpoint.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include "cmdcombineboardnetpoints.h"
#include "cmdcombineboardnetsegments.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineAllItemsUnderBoardNetPoint::CmdCombineAllItemsUnderBoardNetPoint(BI_NetPoint& netpoint) noexcept :
    UndoCommandGroup(tr("Combine Board Items")),
    mCircuit(netpoint.getCircuit()), mBoard(netpoint.getBoard()), mNetPoint(netpoint),
    mHasCombinedSomeItems(false)
{
}

CmdCombineAllItemsUnderBoardNetPoint::~CmdCombineAllItemsUnderBoardNetPoint() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineAllItemsUnderBoardNetPoint::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all vias, netpoints, netlines and footprint pads under the netpoint
    QList<BI_NetPoint*> netpointsUnderCursor = mBoard.getNetPointsAtScenePos(
        mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignalOfNetSegment());
    QList<BI_NetLine*> netlinesUnderCursor = mBoard.getNetLinesAtScenePos(
        mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignalOfNetSegment());
    QList<BI_FootprintPad*> padsUnderCursor = mBoard.getPadsAtScenePos(
        mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignalOfNetSegment());
    QList<BI_Via*> viasUnderCursor = mBoard.getViasAtScenePos(
        mNetPoint.getPosition(), &mNetPoint.getNetSignalOfNetSegment());

    // get all other netsegments/netsignals of the items under the netpoint
    QSet<BI_NetSegment*> netSegmentsUnderCursor;
    QSet<NetSignal*> netSignalsUnderCursor;
    foreach (BI_NetPoint* netpoint, netpointsUnderCursor) {
        netSegmentsUnderCursor.insert(&netpoint->getNetSegment());
        netSignalsUnderCursor.insert(&netpoint->getNetSignalOfNetSegment());
    }
    foreach (BI_NetLine* netline, netlinesUnderCursor) {
        netSegmentsUnderCursor.insert(&netline->getNetSegment());
        netSignalsUnderCursor.insert(&netline->getNetSignalOfNetSegment());
    }
    foreach (BI_FootprintPad* pad, padsUnderCursor) {
        NetSignal* signal = pad->getCompSigInstNetSignal();
        if (signal) { netSignalsUnderCursor.insert(signal); }
    }
    foreach (BI_Via* via, viasUnderCursor) {
        netSegmentsUnderCursor.insert(&via->getNetSegment());
        netSignalsUnderCursor.insert(&via->getNetSignalOfNetSegment());
    }

    // abort if multiple netsignals
    if (netSignalsUnderCursor.count() > 1) {
        throw RuntimeError(__FILE__, __LINE__, tr("Cannot combine board elements because"
            "there are different net signals under the cursor."));
    }

    // combine all netsegments together
    BI_NetSegment& resultingNetSegment = mNetPoint.getNetSegment();
    foreach (BI_NetSegment* netsegment, netSegmentsUnderCursor) { Q_ASSERT(netsegment);
        if (netsegment != &resultingNetSegment) {
            execNewChildCmd(new CmdCombineBoardNetSegments(*netsegment, mNetPoint)); // can throw
            mHasCombinedSomeItems = true;
        }
    }

    // combine netpoints & netlines of the same netsegment under the cursor
    netpointsUnderCursor.clear();
    resultingNetSegment.getNetPointsAtScenePos(mNetPoint.getPosition(),
                                               &mNetPoint.getLayer(), netpointsUnderCursor);
    netpointsUnderCursor.removeOne(&mNetPoint);
    if (netpointsUnderCursor.count() > 0) {
        foreach (BI_NetPoint* netpoint, netpointsUnderCursor) {
            execNewChildCmd(new CmdCombineBoardNetPoints(*netpoint, mNetPoint)); // can throw
            mHasCombinedSomeItems = true;
        }
    } else {
        netlinesUnderCursor.clear();
        resultingNetSegment.getNetLinesAtScenePos(mNetPoint.getPosition(),
                                                  &mNetPoint.getLayer(), netlinesUnderCursor);
        QList<BI_NetLine*> netlinesOfNetpoint = mNetPoint.getLines();
        foreach (BI_NetLine* netline, netlinesUnderCursor) {
            if (!netlinesOfNetpoint.contains(netline)) {
                // TODO: do not create redundant netlines!
                auto* cmdAdd = new CmdBoardNetSegmentAddElements(resultingNetSegment);
                auto* cmdRemove = new CmdBoardNetSegmentRemoveElements(resultingNetSegment);
                cmdRemove->removeNetLine(*netline);
                cmdAdd->addNetLine(mNetPoint, netline->getStartPoint(), netline->getWidth());
                cmdAdd->addNetLine(mNetPoint, netline->getEndPoint(), netline->getWidth());
                execNewChildCmd(cmdAdd); // can throw
                execNewChildCmd(cmdRemove); // can throw
                mHasCombinedSomeItems = true;
            }
        }
    }

    // TODO: connect all pads under the cursor to the netpoint
    if (padsUnderCursor.count() + viasUnderCursor.count() > 1) {
        throw RuntimeError(__FILE__, __LINE__, tr("Sorry, not yet implemented..."));
    } else if (padsUnderCursor.count() == 1) {
        BI_FootprintPad* pad = padsUnderCursor.first();
        if (mNetPoint.getFootprintPad() != pad) {
            if (mNetPoint.getFootprintPad() == nullptr) {
                // attach netpoint to pad
                BI_NetSegment& netsegment = mNetPoint.getNetSegment();
                execNewChildCmd(new CmdBoardNetSegmentRemove(netsegment)); // can throw
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
                cmd->setPadToAttach(pad);
                execNewChildCmd(cmd); // can throw
                execNewChildCmd(new CmdBoardNetSegmentAdd(netsegment)); // can throw
                mHasCombinedSomeItems = true;
            } else {
                throw RuntimeError(__FILE__, __LINE__,
                                   tr("Sorry, not yet implemented..."));
            }
        }
    } else if (viasUnderCursor.count() == 1) {
        BI_Via* via = viasUnderCursor.first();
        if (mNetPoint.getVia() != via) {
            if (mNetPoint.getVia() == nullptr) {
                // attach netpoint to via
                BI_NetSegment& netsegment = mNetPoint.getNetSegment();
                execNewChildCmd(new CmdBoardNetSegmentRemove(netsegment)); // can throw
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
                cmd->setViaToAttach(via);
                execNewChildCmd(cmd); // can throw
                execNewChildCmd(new CmdBoardNetSegmentAdd(netsegment)); // can throw
                mHasCombinedSomeItems = true;
            } else {
                throw RuntimeError(__FILE__, __LINE__,
                                   tr("Sorry, not yet implemented..."));
            }
        }
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
