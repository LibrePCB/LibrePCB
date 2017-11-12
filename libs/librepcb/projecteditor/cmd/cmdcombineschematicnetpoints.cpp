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
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabelremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeladd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>

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

    // check whether both netpoints have the same netsegment
    if (mNetPointToBeRemoved.getNetSegment() != mResultingNetPoint.getNetSegment())
        throw LogicError(__FILE__, __LINE__);

    // change netpoint of all affected netlines
    // TODO: do not create redundant netlines!
    auto* cmdAdd = new CmdSchematicNetSegmentAddElements(mResultingNetPoint.getNetSegment());
    auto* cmdRemove = new CmdSchematicNetSegmentRemoveElements(mResultingNetPoint.getNetSegment());
    foreach (SI_NetLine* line, mNetPointToBeRemoved.getLines()) {
        SI_NetPoint* otherPoint = line->getOtherPoint(mNetPointToBeRemoved); Q_ASSERT(otherPoint);
        cmdRemove->removeNetLine(*line);
        if (otherPoint != &mResultingNetPoint) {
            cmdAdd->addNetLine(mResultingNetPoint, *otherPoint);
        }
    }

    // remove the unused netpoint
    cmdRemove->removeNetPoint(mNetPointToBeRemoved);

    // execute undo commands
    execNewChildCmd(cmdAdd); // can throw
    execNewChildCmd(cmdRemove); // can throw

    // re-connect symbol pin if required
    if (mNetPointToBeRemoved.isAttachedToPin()) {
        SI_SymbolPin* pin = mNetPointToBeRemoved.getSymbolPin(); Q_ASSERT(pin);
        if (!mResultingNetPoint.isAttachedToPin()) {
            Q_ASSERT(!mResultingNetPoint.getSymbolPin());
            execNewChildCmd(new CmdSchematicNetSegmentRemove(mResultingNetPoint.getNetSegment())); // can throw
            CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(mResultingNetPoint);
            cmd->setPinToAttach(pin);
            execNewChildCmd(cmd); // can throw
            execNewChildCmd(new CmdSchematicNetSegmentAdd(mResultingNetPoint.getNetSegment())); // can throw
        } else {
            throw RuntimeError(__FILE__, __LINE__, tr("Could not combine two "
                "schematic netpoints because both are attached to a symbol pin."));
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
