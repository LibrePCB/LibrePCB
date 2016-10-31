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
#include "cmdplaceboardnetpoint.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/common/boardlayer.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
#include "cmdcombineallitemsunderboardnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdPlaceBoardNetPoint::CmdPlaceBoardNetPoint(Board& board, const Point& pos,
                                             BoardLayer& layer) noexcept :
    UndoCommandGroup(tr("Place Board Netpoint")),
    mCircuit(board.getProject().getCircuit()), mBoard(board), mPosition(pos),
    mLayer(layer), mNetPoint(nullptr)
{
}

CmdPlaceBoardNetPoint::~CmdPlaceBoardNetPoint() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdPlaceBoardNetPoint::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all netpoints of the specified layer at the specified position
    QList<BI_NetPoint*> netpointsUnderCursor = mBoard.getNetPointsAtScenePos(mPosition, &mLayer, nullptr);

    if (netpointsUnderCursor.count() == 0) {
        mNetPoint = createNewNetPoint(); // can throw
    } else if (netpointsUnderCursor.count() == 1) {
        mNetPoint = netpointsUnderCursor.first();
    } else {
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("Sorry, not yet implemented..."));
    }
    Q_ASSERT(mNetPoint);

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BI_NetPoint* CmdPlaceBoardNetPoint::createNewNetPoint() throw (Exception)
{
    // get vias at given position
    QList<BI_Via*> viasUnderCursor = mBoard.getViasAtScenePos(mPosition, nullptr);
    if (viasUnderCursor.count() == 0) {
        return createNewNetPointAtPad();
    } else if (viasUnderCursor.count() == 1) {
        BI_Via* via = viasUnderCursor.first();
        BI_NetPoint* netpoint = via->getNetPointOfLayer(mLayer.getId());
        if (netpoint) {
            return netpoint;
        } else {
            NetSignal* netsignal = via->getNetSignal();
            if (!netsignal) {
                throw RuntimeError(__FILE__, __LINE__, QString(), tr("The via is not connected to any net."));
            }
            CmdBoardNetPointAdd* cmd = new CmdBoardNetPointAdd(mBoard, mLayer, *netsignal, *via);
            execNewChildCmd(cmd); // can throw
            return cmd->getNetPoint();
        }
    } else {
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("Sorry, not yet implemented..."));
    }
}

BI_NetPoint* CmdPlaceBoardNetPoint::createNewNetPointAtPad() throw (Exception)
{
    // get pads at given position
    QList<BI_FootprintPad*> padsUnderCursor = mBoard.getPadsAtScenePos(mPosition, &mLayer, nullptr);

    if (padsUnderCursor.count() == 0) {
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("No pads or vias at given position."));
    } else if (padsUnderCursor.count() == 1) {
        BI_FootprintPad* pad = padsUnderCursor.first();
        NetSignal* netsignal = pad->getCompSigInstNetSignal();
        if (!netsignal) {
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("The pin is not connected to any net."));
        }
        CmdBoardNetPointAdd* cmd = new CmdBoardNetPointAdd(mBoard, mLayer, *netsignal, *pad);
        execNewChildCmd(cmd); // can throw
        return cmd->getNetPoint();
    } else {
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("Sorry, not yet implemented..."));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
