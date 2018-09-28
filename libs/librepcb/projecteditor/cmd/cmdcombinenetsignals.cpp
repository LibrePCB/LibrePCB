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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdcombinenetsignals.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardplaneadd.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include <librepcb/project/boards/cmd/cmdboardplaneremove.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetsignalremove.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremove.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netsegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdCombineNetSignals::CmdCombineNetSignals(Circuit&   circuit,
                                           NetSignal& toBeRemoved,
                                           NetSignal& result) noexcept
  : UndoCommandGroup(tr("Combine Net Signals")),
    mCircuit(circuit),
    mNetSignalToRemove(toBeRemoved),
    mResultingNetSignal(result) {
}

CmdCombineNetSignals::~CmdCombineNetSignals() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineNetSignals::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // determine all elements which need to be removed temporary
  QList<SI_NetSegment*> schematicNetSegments =
      mNetSignalToRemove.getSchematicNetSegments();
  QList<BI_NetSegment*> boardNetSegments =
      mNetSignalToRemove.getBoardNetSegments();
  QList<BI_Plane*> boardPlanes = mNetSignalToRemove.getBoardPlanes();

  // remove all schematic netsegments
  foreach (SI_NetSegment* netsegment, schematicNetSegments) {
    execNewChildCmd(new CmdSchematicNetSegmentRemove(*netsegment));
  }

  // remove all board netsegments
  foreach (BI_NetSegment* netsegment, boardNetSegments) {
    execNewChildCmd(new CmdBoardNetSegmentRemove(*netsegment));  // can throw
  }

  // remove all board planes
  foreach (BI_Plane* plane, boardPlanes) {
    execNewChildCmd(new CmdBoardPlaneRemove(*plane));  // can throw
  }

  // change netsignal of all component signal instances
  foreach (ComponentSignalInstance* signal,
           mNetSignalToRemove.getComponentSignals()) {
    execNewChildCmd(new CmdCompSigInstSetNetSignal(
        *signal, &mResultingNetSignal));  // can throw
  }

  // re-add all board netsegments
  foreach (BI_NetSegment* netsegment, boardNetSegments) {
    CmdBoardNetSegmentEdit* cmd = new CmdBoardNetSegmentEdit(*netsegment);
    cmd->setNetSignal(mResultingNetSignal);
    execNewChildCmd(cmd);                                     // can throw
    execNewChildCmd(new CmdBoardNetSegmentAdd(*netsegment));  // can throw
  }

  // re-add all board planes
  foreach (BI_Plane* plane, boardPlanes) {
    CmdBoardPlaneEdit* cmd = new CmdBoardPlaneEdit(*plane, false);
    cmd->setNetSignal(mResultingNetSignal);
    execNewChildCmd(cmd);                           // can throw
    execNewChildCmd(new CmdBoardPlaneAdd(*plane));  // can throw
  }

  // re-add all schematic netsegments
  foreach (SI_NetSegment* netsegment, schematicNetSegments) {
    auto* cmd = new CmdSchematicNetSegmentEdit(*netsegment);
    cmd->setNetSignal(mResultingNetSignal);
    execNewChildCmd(cmd);
    execNewChildCmd(new CmdSchematicNetSegmentAdd(*netsegment));
  }

  // remove the old netsignal
  execNewChildCmd(
      new CmdNetSignalRemove(mCircuit, mNetSignalToRemove));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
