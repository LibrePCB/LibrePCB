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
#include "cmdschematicnetlabelremove.h"

#include <librepcb/core/project/schematic/items/si_netlabel.h>
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

CmdSchematicNetLabelRemove::CmdSchematicNetLabelRemove(
    SI_NetLabel& netlabel) noexcept
  : UndoCommand(tr("Remove netlabel")),
    mNetSegment(netlabel.getNetSegment()),
    mNetLabel(netlabel) {
}

CmdSchematicNetLabelRemove::~CmdSchematicNetLabelRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicNetLabelRemove::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdSchematicNetLabelRemove::performUndo() {
  mNetSegment.addNetLabel(mNetLabel);  // can throw
}

void CmdSchematicNetLabelRemove::performRedo() {
  mNetSegment.removeNetLabel(mNetLabel);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
