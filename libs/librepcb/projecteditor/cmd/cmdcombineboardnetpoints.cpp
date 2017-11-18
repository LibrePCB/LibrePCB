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
#include "cmdcombineboardnetpoints.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineBoardNetPoints::CmdCombineBoardNetPoints(BI_NetPoint& toBeRemoved,
                                                   BI_NetPoint& result) noexcept :
    UndoCommandGroup(tr("Combine Board Netpoints")),
    mNetPointToBeRemoved(toBeRemoved), mResultingNetPoint(result)
{
}

CmdCombineBoardNetPoints::~CmdCombineBoardNetPoints() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineBoardNetPoints::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // check whether both netpoints have the same netsegment
    if (mNetPointToBeRemoved.getNetSegment() != mResultingNetPoint.getNetSegment())
        throw LogicError(__FILE__, __LINE__);

    // change netpoint of all affected netlines
    // TODO: do not create redundant netlines!
    auto* cmdAdd = new CmdBoardNetSegmentAddElements(mResultingNetPoint.getNetSegment());
    auto* cmdRemove = new CmdBoardNetSegmentRemoveElements(mResultingNetPoint.getNetSegment());
    foreach (BI_NetLine* line, mNetPointToBeRemoved.getLines()) {
        BI_NetPoint* otherPoint = line->getOtherPoint(mNetPointToBeRemoved); Q_ASSERT(otherPoint);
        cmdRemove->removeNetLine(*line);
        if (otherPoint != &mResultingNetPoint) {
            cmdAdd->addNetLine(mResultingNetPoint, *otherPoint, line->getWidth());
        }
    }

    // remove the unused netpoint
    cmdRemove->removeNetPoint(mNetPointToBeRemoved);

    // execute undo commands
    execNewChildCmd(cmdAdd); // can throw
    execNewChildCmd(cmdRemove); // can throw

    // re-connect pads and vias if required
    if (mNetPointToBeRemoved.isAttachedToPad()) {
        BI_FootprintPad* pad = mNetPointToBeRemoved.getFootprintPad(); Q_ASSERT(pad);
        if (!mResultingNetPoint.isAttached()) {
            Q_ASSERT(!mResultingNetPoint.getFootprintPad());
            execNewChildCmd(new CmdBoardNetSegmentRemove(mResultingNetPoint.getNetSegment())); // can throw
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mResultingNetPoint);
            cmd->setPadToAttach(pad);
            execNewChildCmd(cmd); // can throw
            execNewChildCmd(new CmdBoardNetSegmentAdd(mResultingNetPoint.getNetSegment())); // can throw
        } else {
            throw RuntimeError(__FILE__, __LINE__, tr("Could not combine two "
                "schematic netpoints because both are attached to a pad or via."));
        }
    } else if (mNetPointToBeRemoved.isAttachedToVia()) {
        BI_Via* via = mNetPointToBeRemoved.getVia(); Q_ASSERT(via);
        if (!mResultingNetPoint.isAttached()) {
            Q_ASSERT(!mResultingNetPoint.getVia());
            execNewChildCmd(new CmdBoardNetSegmentRemove(mResultingNetPoint.getNetSegment())); // can throw
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mResultingNetPoint);
            cmd->setViaToAttach(via);
            execNewChildCmd(cmd); // can throw
            execNewChildCmd(new CmdBoardNetSegmentAdd(mResultingNetPoint.getNetSegment())); // can throw
        } else {
            throw RuntimeError(__FILE__, __LINE__, tr("Could not combine two "
                "schematic netpoints because both are attached to a pad or via."));
        }
    }

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
