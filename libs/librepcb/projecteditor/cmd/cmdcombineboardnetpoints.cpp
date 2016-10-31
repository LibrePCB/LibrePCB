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
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointremove.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

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

bool CmdCombineBoardNetPoints::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // TODO: do not create redundant netlines!

    // change netpoint of all affected netlines
    foreach (BI_NetLine* line, mNetPointToBeRemoved.getLines()) {
        BI_NetPoint* otherPoint;
        if (&line->getStartPoint() == &mNetPointToBeRemoved) {
            otherPoint = &line->getEndPoint();
        } else if (&line->getEndPoint() == &mNetPointToBeRemoved) {
            otherPoint = &line->getStartPoint();
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
        // TODO: maybe add the ability to change start-/endpoint of lines instead
        //       of remove the line and add a completely new line (attributes are lost!)
        execNewChildCmd(new CmdBoardNetLineRemove(*line)); // can throw
        if (otherPoint != &mResultingNetPoint) {
            execNewChildCmd(new CmdBoardNetLineAdd(line->getBoard(), mResultingNetPoint,
                                                   *otherPoint, line->getWidth())); // can throw
        }
    }

    // remove the unused netpoint
    execNewChildCmd(new CmdBoardNetPointRemove(mNetPointToBeRemoved)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
