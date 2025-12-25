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
#include "cmdcombinebuses.h"

#include "../../project/cmd/cmdbusedit.h"
#include "../../project/cmd/cmdbusremove.h"
#include "../../project/cmd/cmdschematicbussegmentadd.h"
#include "../../project/cmd/cmdschematicbussegmentedit.h"
#include "../../project/cmd/cmdschematicbussegmentremove.h"
#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static std::optional<UnsignedLength> mergeMinTraceLengthDifference(
    const std::optional<UnsignedLength>& a,
    const std::optional<UnsignedLength>& b) noexcept {
  if (a && b) {
    return std::min(*a, *b);
  } else if (a) {
    return a;
  } else {
    return b;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdCombineBuses::CmdCombineBuses(Circuit& circuit, Bus& toBeRemoved,
                                 Bus& result) noexcept
  : UndoCommandGroup(tr("Combine Net Signals")),
    mCircuit(circuit),
    mBusToRemove(toBeRemoved),
    mResultingBus(result) {
}

CmdCombineBuses::~CmdCombineBuses() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineBuses::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Determine all elements which need to be removed temporary.
  QList<SI_BusSegment*> busSegments = mBusToRemove.getSchematicBusSegments();
  QSet<SI_NetSegment*> netSegments;
  for (SI_BusSegment* bs : busSegments) {
    netSegments |= bs->getAttachedNetSegments();
  }

  // Remove net segments & bus segments.
  foreach (SI_NetSegment* ns, netSegments) {
    execNewChildCmd(new CmdSchematicNetSegmentRemove(*ns));
  }
  foreach (SI_BusSegment* bs, busSegments) {
    execNewChildCmd(new CmdSchematicBusSegmentRemove(*bs));
  }

  // Merge bus properties.
  CmdBusEdit* cmdEdit = new CmdBusEdit(mResultingBus);
  cmdEdit->setPrefixNetNames(mResultingBus.getPrefixNetNames() ||
                             mBusToRemove.getPrefixNetNames());
  cmdEdit->setMaxTraceLengthDifference(mergeMinTraceLengthDifference(
      mResultingBus.getMaxTraceLengthDifference(),
      mBusToRemove.getMaxTraceLengthDifference()));
  execNewChildCmd(cmdEdit);

  // Re-add all bus & net segments.
  foreach (SI_BusSegment* bs, busSegments) {
    CmdSchematicBusSegmentEdit* cmd = new CmdSchematicBusSegmentEdit(*bs);
    cmd->setBus(mResultingBus);
    execNewChildCmd(cmd);
    execNewChildCmd(new CmdSchematicBusSegmentAdd(*bs));
  }
  foreach (SI_NetSegment* ns, netSegments) {
    execNewChildCmd(new CmdSchematicNetSegmentAdd(*ns));
  }

  // Remove the old bus.
  execNewChildCmd(new CmdBusRemove(mCircuit, mBusToRemove));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
