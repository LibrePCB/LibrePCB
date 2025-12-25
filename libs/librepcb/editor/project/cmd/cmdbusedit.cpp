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
#include "cmdbusedit.h"

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

CmdBusEdit::CmdBusEdit(Bus& bus) noexcept
  : UndoCommand(tr("Edit Bus")),
    mCircuit(bus.getCircuit()),
    mBus(bus),
    mOldName(bus.getName()),
    mNewName(mOldName),
    mOldIsAutoName(bus.hasAutoName()),
    mNewIsAutoName(mOldIsAutoName),
    mOldPrefixNetNames(bus.getPrefixNetNames()),
    mNewPrefixNetNames(mOldPrefixNetNames),
    mOldMaxTraceLengthDifference(bus.getMaxTraceLengthDifference()),
    mNewMaxTraceLengthDifference(mOldMaxTraceLengthDifference) {
}

CmdBusEdit::~CmdBusEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBusEdit::setName(const BusName& name, bool isAutoName) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
  mNewIsAutoName = isAutoName;
}

void CmdBusEdit::setPrefixNetNames(bool prefix) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPrefixNetNames = prefix;
}

void CmdBusEdit::setMaxTraceLengthDifference(
    const std::optional<UnsignedLength>& diff) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMaxTraceLengthDifference = diff;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBusEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewIsAutoName != mOldIsAutoName) return true;
  if (mNewPrefixNetNames != mOldPrefixNetNames) return true;
  if (mNewMaxTraceLengthDifference != mOldMaxTraceLengthDifference) return true;
  return false;
}

void CmdBusEdit::performUndo() {
  if ((mNewName != mOldName) || (mNewIsAutoName != mOldIsAutoName)) {
    mCircuit.setBusName(mBus, mOldName, mOldIsAutoName);  // can throw
  }
  mBus.setPrefixNetNames(mOldPrefixNetNames);
  mBus.setMaxTraceLengthDifference(mOldMaxTraceLengthDifference);
}

void CmdBusEdit::performRedo() {
  if ((mNewName != mOldName) || (mNewIsAutoName != mOldIsAutoName)) {
    mCircuit.setBusName(mBus, mNewName, mNewIsAutoName);  // can throw
  }
  mBus.setPrefixNetNames(mNewPrefixNetNames);
  mBus.setMaxTraceLengthDifference(mNewMaxTraceLengthDifference);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
