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
#include "cmdbusadd.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/circuit/circuit.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBusAdd::CmdBusAdd(Circuit& circuit) noexcept
  : UndoCommand(tr("Add Bus")), mCircuit(circuit), mBus(nullptr) {
}

CmdBusAdd::CmdBusAdd(Bus& bus) noexcept
  : UndoCommand(tr("Add Bus")), mCircuit(bus.getCircuit()), mBus(&bus) {
}

CmdBusAdd::~CmdBusAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBusAdd::performExecute() {
  if (!mBus) {
    // Add new bus with auto name
    mBus = new Bus(mCircuit, Uuid::createRandom(),
                   BusName(mCircuit.generateAutoBusName()), true, false,
                   std::nullopt);  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdBusAdd::performUndo() {
  mCircuit.removeBus(*mBus);  // can throw
}

void CmdBusAdd::performRedo() {
  mCircuit.addBus(*mBus);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
