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
#include "cmdschematicbussegmentedit.h"

#include <librepcb/core/project/schematic/items/si_bussegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicBusSegmentEdit::CmdSchematicBusSegmentEdit(
    SI_BusSegment& segment) noexcept
  : UndoCommand(tr("Edit Bus Segment")),
    mSegment(segment),
    mOldBus(&segment.getBus()),
    mNewBus(mOldBus) {
}

CmdSchematicBusSegmentEdit::~CmdSchematicBusSegmentEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdSchematicBusSegmentEdit::setBus(Bus& bus) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewBus = &bus;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicBusSegmentEdit::performExecute() {
  performRedo();  // can throw

  return (mNewBus != mOldBus);
}

void CmdSchematicBusSegmentEdit::performUndo() {
  mSegment.setBus(*mOldBus);  // can throw
}

void CmdSchematicBusSegmentEdit::performRedo() {
  mSegment.setBus(*mNewBus);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
