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
#include "cmdremoveunusednetsignals.h"

#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardplaneremove.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdnetsignalremove.h>
#include <librepcb/project/circuit/netsignal.h>

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

CmdRemoveUnusedNetSignals::CmdRemoveUnusedNetSignals(Circuit& circuit) noexcept
  : UndoCommandGroup(tr("Remove Unused Net Signals")), mCircuit(circuit) {
}

CmdRemoveUnusedNetSignals::~CmdRemoveUnusedNetSignals() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveUnusedNetSignals::performExecute() {
  foreach (NetSignal* netsignal, mCircuit.getNetSignals()) {
    bool noComponentSignals = netsignal->getComponentSignals().isEmpty();
    bool noSchematicNetSegments =
        netsignal->getSchematicNetSegments().isEmpty();
    if (noComponentSignals && noSchematicNetSegments) {
      foreach (BI_NetSegment* netsegment, netsignal->getBoardNetSegments()) {
        appendChild(new CmdBoardNetSegmentRemove(*netsegment));
      }
      foreach (BI_Plane* plane, netsignal->getBoardPlanes()) {
        appendChild(new CmdBoardPlaneRemove(*plane));
      }
      appendChild(new CmdNetSignalRemove(mCircuit, *netsignal));
    }
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
