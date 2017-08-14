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
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
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
    QList<BI_NetPoint*> netpointsUnderCursor = mBoard.getNetPointsAtScenePos(mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignal());
    QList<BI_NetLine*> netlinesUnderCursor = mBoard.getNetLinesAtScenePos(mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignal());
    QList<BI_FootprintPad*> padsUnderCursor = mBoard.getPadsAtScenePos(mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignal());
    QList<BI_Via*> viasUnderCursor = mBoard.getViasAtScenePos(mNetPoint.getPosition(), &mNetPoint.getNetSignal());

    // combine all netpoints together
    // TODO: does this work properly in any case?
    foreach (BI_NetPoint* netpoint, netpointsUnderCursor) {
        if (netpoint != &mNetPoint) {
            execNewChildCmd(new CmdCombineBoardNetPoints(*netpoint, mNetPoint)); // can throw
            mHasCombinedSomeItems = true;
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
                QList<BI_NetLine*> lines = mNetPoint.getLines();
                foreach (BI_NetLine* line, lines) {
                    execNewChildCmd(new CmdBoardNetLineRemove(*line)); // can throw
                }
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
                cmd->setPadToAttach(pad);
                execNewChildCmd(cmd); // can throw
                foreach (BI_NetLine* line, lines) {
                    execNewChildCmd(new CmdBoardNetLineAdd(*line)); // can throw
                }
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
                QList<BI_NetLine*> lines = mNetPoint.getLines();
                foreach (BI_NetLine* line, lines) {
                    execNewChildCmd(new CmdBoardNetLineRemove(*line)); // can throw
                }
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
                cmd->setViaToAttach(via);
                execNewChildCmd(cmd); // can throw
                foreach (BI_NetLine* line, lines) {
                    execNewChildCmd(new CmdBoardNetLineAdd(*line)); // can throw
                }
                mHasCombinedSomeItems = true;
            } else {
                throw RuntimeError(__FILE__, __LINE__,
                                   tr("Sorry, not yet implemented..."));
            }
        }
    }

    // split all lines under the cursor and connect them to the netpoint
    // TODO: avoid adding duplicate netlines!
    netlinesUnderCursor = mBoard.getNetLinesAtScenePos(mNetPoint.getPosition(), &mNetPoint.getLayer(), &mNetPoint.getNetSignal()); // important!
    foreach (BI_NetLine* netline, netlinesUnderCursor) {
        BI_NetPoint& p1 = netline->getStartPoint();
        BI_NetPoint& p2 = netline->getEndPoint();
        if ((p1 != mNetPoint) && (p2 != mNetPoint)) {
            execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
            execNewChildCmd(new CmdBoardNetLineAdd(mBoard, p1, mNetPoint, netline->getWidth())); // can throw
            execNewChildCmd(new CmdBoardNetLineAdd(mBoard, mNetPoint, p2, netline->getWidth())); // can throw
            mHasCombinedSomeItems = true;
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
