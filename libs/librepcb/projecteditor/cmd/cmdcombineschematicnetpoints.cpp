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
#include "cmdcombineschematicnetpoints.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointremove.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineSchematicNetPoints::CmdCombineSchematicNetPoints(SI_NetPoint& toBeRemoved,
                                                           SI_NetPoint& result) noexcept :
    UndoCommandGroup(tr("Combine Schematic Netpoints")),
    mNetPointToBeRemoved(toBeRemoved), mResultingNetPoint(result)
{
}

CmdCombineSchematicNetPoints::~CmdCombineSchematicNetPoints() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineSchematicNetPoints::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // TODO: do not create redundant netlines!

    // change netpoint of all affected netlines
    foreach (SI_NetLine* line, mNetPointToBeRemoved.getLines()) {
        SI_NetPoint* otherPoint;
        if (&line->getStartPoint() == &mNetPointToBeRemoved) {
            otherPoint = &line->getEndPoint();
        } else if (&line->getEndPoint() == &mNetPointToBeRemoved) {
            otherPoint = &line->getStartPoint();
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
        // TODO: maybe add the ability to change start-/endpoint of lines instead
        //       of remove the line and add a completely new line (attributes are lost!)
        execNewChildCmd(new CmdSchematicNetLineRemove(*line)); // can throw
        if (otherPoint != &mResultingNetPoint) {
            execNewChildCmd(new CmdSchematicNetLineAdd(line->getSchematic(), mResultingNetPoint, *otherPoint)); // can throw
        }
    }

    // remove the unused netpoint
    execNewChildCmd(new CmdSchematicNetPointRemove(mNetPointToBeRemoved)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
