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
#include "cmdschematicnetsegmentadd.h"

#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicNetSegmentAdd::CmdSchematicNetSegmentAdd(
    SI_NetSegment& segment) noexcept
  : UndoCommand(tr("Add net segment")),
    mSchematic(segment.getSchematic()),
    mNetSignal(segment.getNetSignal()),
    mNetSegment(&segment) {
}

CmdSchematicNetSegmentAdd::CmdSchematicNetSegmentAdd(
    Schematic& schematic, NetSignal& netsignal) noexcept
  : UndoCommand(tr("Add net segment")),
    mSchematic(schematic),
    mNetSignal(netsignal),
    mNetSegment(nullptr) {
}

CmdSchematicNetSegmentAdd::~CmdSchematicNetSegmentAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicNetSegmentAdd::performExecute() {
  if (!mNetSegment) {
    // create new net segment
    mNetSegment = new SI_NetSegment(mSchematic, mNetSignal);  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdSchematicNetSegmentAdd::performUndo() {
  mSchematic.removeNetSegment(*mNetSegment);  // can throw
}

void CmdSchematicNetSegmentAdd::performRedo() {
  mSchematic.addNetSegment(*mNetSegment);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
