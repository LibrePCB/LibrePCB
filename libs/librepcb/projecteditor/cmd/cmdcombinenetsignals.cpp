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
#include "cmdcombinenetsignals.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardviaadd.h>
#include <librepcb/project/boards/cmd/cmdboardviaremove.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/circuit/cmd/cmdnetsignalremove.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeledit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

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

bool CmdCombineNetSignals::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // change netsignal of all netlabels
    foreach (SI_NetLabel* label, mNetSignalToRemove.getSchematicNetLabels()) {
        auto* cmd = new CmdSchematicNetLabelEdit(*label);
        cmd->setNetSignal(mResultingNetSignal, false);
        execNewChildCmd(cmd); // can throw
    }

    // determine all schematic elements which need to be removed temporary
    QList<SI_NetPoint*> schematicNetPoints = mNetSignalToRemove.getSchematicNetPoints();
    QSet<SI_NetLine*> schematicNetLines;
    foreach (SI_NetPoint* point, schematicNetPoints) {
        foreach (SI_NetLine* line, point->getLines()) {
            schematicNetLines.insert(line);
        }
    }

    // determine all board elements which need to be removed temporary
    QList<BI_Via*> boardVias = mNetSignalToRemove.getBoardVias();
    QList<BI_NetPoint*> boardNetPoints = mNetSignalToRemove.getBoardNetPoints();
    QSet<BI_NetLine*> boardNetLines;
    foreach (BI_NetPoint* netpoint, boardNetPoints) { Q_ASSERT(netpoint);
        foreach (BI_NetLine* netline, netpoint->getLines()) { Q_ASSERT(netline);
            boardNetLines.insert(netline);
        }
    }

    // remove all schematic netlines
    foreach (SI_NetLine* netline, schematicNetLines) {
        execNewChildCmd(new CmdSchematicNetLineRemove(*netline)); // can throw
    }

    // remove all schematic netpoints
    foreach (SI_NetPoint* netpoint, schematicNetPoints) {
        execNewChildCmd(new CmdSchematicNetPointRemove(*netpoint)); // can throw
    }

    // remove all board netlines
    foreach (BI_NetLine* netline, boardNetLines) {
        execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
    }

    // remove all board netpoints
    foreach (BI_NetPoint* netpoint, boardNetPoints) {
        execNewChildCmd(new CmdBoardNetPointRemove(*netpoint)); // can throw
    }

    // remove all board vias
    foreach (BI_Via* via, boardVias) {
        execNewChildCmd(new CmdBoardViaRemove(*via)); // can throw
    }

    // change netsignal of all component signal instances
    foreach (ComponentSignalInstance* signal, mNetSignalToRemove.getComponentSignals()) {
        execNewChildCmd(new CmdCompSigInstSetNetSignal(*signal, &mResultingNetSignal)); // can throw
    }

    // re-add all board vias
    foreach (BI_Via* via, boardVias) {
        CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
        cmd->setNetSignal(&mResultingNetSignal, false);
        execNewChildCmd(cmd); // can throw
        execNewChildCmd(new CmdBoardViaAdd(*via)); // can throw
    }

    // re-add all board netpoints
    foreach (BI_NetPoint* netpoint, boardNetPoints) {
        CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
        cmd->setNetSignal(mResultingNetSignal);
        execNewChildCmd(cmd); // can throw
        execNewChildCmd(new CmdBoardNetPointAdd(*netpoint)); // can throw
    }

    // re-add all board netlines
    foreach (BI_NetLine* netline, boardNetLines) {
        execNewChildCmd(new CmdBoardNetLineAdd(*netline)); // can throw
    }

    // re-add all schematic netpoints
    foreach (SI_NetPoint* netpoint, schematicNetPoints) {
        CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
        cmd->setNetSignal(mResultingNetSignal);
        execNewChildCmd(cmd); // can throw
        execNewChildCmd(new CmdSchematicNetPointAdd(*netpoint)); // can throw
    }

    // re-add all schematic netlines
    foreach (SI_NetLine* netline, schematicNetLines) {
        execNewChildCmd(new CmdSchematicNetLineAdd(*netline)); // can throw
    }

    // remove the old netsignal
    execNewChildCmd(new CmdNetSignalRemove(mCircuit, mNetSignalToRemove)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
