/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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
#include "cmdchangenetsignalofschematicnetsegment.h"

#include "../../project/cmd/cmdcompsiginstsetnetsignal.h"
#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentedit.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"
#include "cmdcombinenetsignals.h"
#include "cmdremoveboarditems.h"

#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdChangeNetSignalOfSchematicNetSegment::
    CmdChangeNetSignalOfSchematicNetSegment(SI_NetSegment& seg,
                                            NetSignal& newSig) noexcept
  : UndoCommandGroup(tr("Change netsignal of netsegment")),
    mNetSegment(seg),
    mNewNetSignal(newSig) {
}

CmdChangeNetSignalOfSchematicNetSegment::
    ~CmdChangeNetSignalOfSchematicNetSegment() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdChangeNetSignalOfSchematicNetSegment::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  if (mNewNetSignal == mNetSegment.getNetSignal()) {
    // nothing to do, the netsignal is already correct
    undoScopeGuard.dismiss();
    return false;
  } else if (mNetSegment.getNetSignal().getSchematicNetSegments().count() ==
             1) {
    // this netsegment is the only one in its netsignal,
    // we just need to combine both netsignals
    Circuit& circuit = mNetSegment.getCircuit();
    NetSignal& toBeRemoved = mNetSegment.getNetSignal();
    NetSignal& result = mNewNetSignal;
    execNewChildCmd(
        new CmdCombineNetSignals(circuit, toBeRemoved, result));  // can throw
  } else {
    // there are still some other netsegments with the same netsignal
    Q_ASSERT(mNetSegment.getNetSignal().getSchematicNetSegments().count() > 1);
    changeNetSignalOfNetSegment();  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

void CmdChangeNetSignalOfSchematicNetSegment::changeNetSignalOfNetSegment() {
  // memorize pins of netsegment
  QSet<SI_SymbolPin*> pins = mNetSegment.getAllConnectedPins();

  // remove netsegment
  execNewChildCmd(new CmdSchematicNetSegmentRemove(mNetSegment));  // can throw

  // set netsignal of netsegment
  CmdSchematicNetSegmentEdit* cmd = new CmdSchematicNetSegmentEdit(mNetSegment);
  cmd->setNetSignal(mNewNetSignal);
  execNewChildCmd(cmd);  // can throw

  // change netsignal of all connected symbol pins (resp. their component
  // signals)
  foreach (SI_SymbolPin* pin, pins) {
    Q_ASSERT(pin);
    ComponentSignalInstance* sig = pin->getComponentSignalInstance();
    if (sig) {
      updateCompSigInstNetSignal(*sig);
    }
  }

  // re-add netsegment
  execNewChildCmd(new CmdSchematicNetSegmentAdd(mNetSegment));  // can throw
}

void CmdChangeNetSignalOfSchematicNetSegment::updateCompSigInstNetSignal(
    ComponentSignalInstance& cmpSig) {
  // disconnect traces from pads in all boards
  QHash<Board*, QSet<BI_NetLine*>> boardNetLinesToRemove;
  foreach (BI_FootprintPad* pad, cmpSig.getRegisteredFootprintPads()) {
    Q_ASSERT(pad && pad->isAddedToBoard());
    boardNetLinesToRemove[&pad->getBoard()] += pad->getNetLines();
  }
  for (auto it = boardNetLinesToRemove.constBegin();
       it != boardNetLinesToRemove.constEnd(); ++it) {
    QScopedPointer<CmdRemoveBoardItems> cmd(new CmdRemoveBoardItems(*it.key()));
    cmd->removeNetLines(it.value());
    execNewChildCmd(cmd.take());  // can throw
  }

  // change netsignal of component signal instance
  execNewChildCmd(
      new CmdCompSigInstSetNetSignal(cmpSig, &mNewNetSignal));  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
