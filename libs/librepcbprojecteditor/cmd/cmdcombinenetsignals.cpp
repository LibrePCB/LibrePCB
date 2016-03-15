/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "cmdcombinenetsignals.h"
#include <librepcbcommon/scopeguard.h>
#include <librepcbproject/circuit/netsignal.h>
#include <librepcbproject/schematics/items/si_netpoint.h>
#include <librepcbproject/schematics/items/si_netline.h>
#include <librepcbproject/circuit/cmd/cmdnetsignalremove.h>
#include <librepcbproject/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlabeledit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineNetSignals::CmdCombineNetSignals(Circuit& circuit, NetSignal& toBeRemoved,
                                           NetSignal& result) noexcept :
    UndoCommandGroup(tr("Combine Net Signals")),
    mCircuit(circuit), mNetSignalToRemove(toBeRemoved), mResultingNetSignal(result)
{
}

CmdCombineNetSignals::~CmdCombineNetSignals() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineNetSignals::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // change netsignal of all netlabels
    foreach (SI_NetLabel* label, mNetSignalToRemove.getNetLabels()) {
        auto* cmd = new CmdSchematicNetLabelEdit(*label);
        cmd->setNetSignal(mResultingNetSignal, false);
        execNewChildCmd(cmd); // can throw
    }

    // disconnect all schematic netlines and netpoints
    QList<SI_NetLine*> removedNetLines;
    QList<SI_NetPoint*> removedNetPoints;
    foreach (SI_NetPoint* point, mNetSignalToRemove.getNetPoints()) {
        foreach (SI_NetLine* line, point->getLines()) {
            execNewChildCmd(new CmdSchematicNetLineRemove(*line)); // can throw
            removedNetLines.append(line);
        }
        execNewChildCmd(new CmdSchematicNetPointRemove(*point)); // can throw
        removedNetPoints.append(point);
    }

    // change netsignal of all component signal instances
    foreach (ComponentSignalInstance* signal, mNetSignalToRemove.getComponentSignals()) {
        execNewChildCmd(new CmdCompSigInstSetNetSignal(*signal, &mResultingNetSignal)); // can throw
    }

    // reconnect all disconnected schematic netpoints
    foreach (SI_NetPoint* point, removedNetPoints) {
        CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*point);
        cmd->setNetSignal(mResultingNetSignal);
        execNewChildCmd(cmd); // can throw
        execNewChildCmd(new CmdSchematicNetPointAdd(*point)); // can throw
    }

    // reconnect all disconnected schematic netlines
    foreach (SI_NetLine* line, removedNetLines) {
        execNewChildCmd(new CmdSchematicNetLineAdd(*line)); // can throw
    }

    // remove the old netsignal
    execNewChildCmd(new CmdNetSignalRemove(mCircuit, mNetSignalToRemove)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
