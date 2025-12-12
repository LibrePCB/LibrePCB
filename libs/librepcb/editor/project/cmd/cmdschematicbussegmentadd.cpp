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
#include "cmdschematicbussegmentadd.h"

#include <librepcb/core/project/schematic/items/si_bussegment.h>
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

CmdSchematicBusSegmentAdd::CmdSchematicBusSegmentAdd(
    SI_BusSegment& segment) noexcept
  : UndoCommand(tr("Add Bus Segment")),
    mSchematic(segment.getSchematic()),
    mBus(&segment.getBus()),
    mSegment(&segment) {
}

CmdSchematicBusSegmentAdd::CmdSchematicBusSegmentAdd(Schematic& schematic,
                                                     Bus& bus) noexcept
  : UndoCommand(tr("Add Bus Segment")),
    mSchematic(schematic),
    mBus(&bus),
    mSegment(nullptr) {
}

CmdSchematicBusSegmentAdd::~CmdSchematicBusSegmentAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicBusSegmentAdd::performExecute() {
  if (!mSegment) {
    // Create new bus segment
    mSegment = new SI_BusSegment(mSchematic, Uuid::createRandom(),
                                 *mBus);  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdSchematicBusSegmentAdd::performUndo() {
  mSchematic.removeBusSegment(*mSegment);  // can throw
}

void CmdSchematicBusSegmentAdd::performRedo() {
  mSchematic.addBusSegment(*mSegment);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
