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
#include "cmdchangebusofschematicbussegment.h"

#include "../../project/cmd/cmdschematicbussegmentadd.h"
#include "../../project/cmd/cmdschematicbussegmentedit.h"
#include "../../project/cmd/cmdschematicbussegmentremove.h"
#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"
#include "cmdcombinebuses.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
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

CmdChangeBusOfSchematicBusSegment::CmdChangeBusOfSchematicBusSegment(
    SI_BusSegment& seg, Bus& newBus) noexcept
  : UndoCommandGroup(tr("Change Bus Of Segment")),
    mSegment(seg),
    mNewBus(newBus) {
}

CmdChangeBusOfSchematicBusSegment::
    ~CmdChangeBusOfSchematicBusSegment() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdChangeBusOfSchematicBusSegment::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  if (mNewBus == mSegment.getBus()) {
    // nothing to do, the netsignal is already correct
    undoScopeGuard.dismiss();
    return false;
  } else if (mSegment.getBus().getSchematicBusSegments().count() == 1) {
    // This bus segment is the only one of its bus,
    // we just need to combine both buses.
    Circuit& circuit = mSegment.getCircuit();
    Bus& toBeRemoved = mSegment.getBus();
    Bus& result = mNewBus;
    execNewChildCmd(
        new CmdCombineBuses(circuit, toBeRemoved, result));  // can throw
  } else {
    // There are still some other segments with the same bus.
    Q_ASSERT(mSegment.getBus().getSchematicBusSegments().count() > 1);
    changeBusOfSegment();  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

void CmdChangeBusOfSchematicBusSegment::changeBusOfSegment() {
  // Remove all attached net segments.
  QSet<SI_NetSegment*> netSegments = mSegment.getAttachedNetSegments();
  for (SI_NetSegment* ns : netSegments) {
    execNewChildCmd(new CmdSchematicNetSegmentRemove(*ns));  // can throw
  }

  // Remove bus segment.
  execNewChildCmd(new CmdSchematicBusSegmentRemove(mSegment));  // can throw

  // Set bus of segment.
  CmdSchematicBusSegmentEdit* cmd = new CmdSchematicBusSegmentEdit(mSegment);
  cmd->setBus(mNewBus);
  execNewChildCmd(cmd);  // can throw

  // Re-add bus segment.
  execNewChildCmd(new CmdSchematicBusSegmentAdd(mSegment));  // can throw

  // Re-add all net segments.
  for (SI_NetSegment* ns : netSegments) {
    execNewChildCmd(new CmdSchematicNetSegmentAdd(*ns));  // can throw
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
