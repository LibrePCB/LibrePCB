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
#include "cmdnetsignaledit.h"

#include "../circuit.h"
#include "../netsignal.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdNetSignalEdit::CmdNetSignalEdit(Circuit& circuit,
                                   NetSignal& netsignal) noexcept
  : UndoCommand(tr("Edit netsignal")),
    mCircuit(circuit),
    mNetSignal(netsignal),
    mOldName(netsignal.getName()),
    mNewName(mOldName),
    mOldIsAutoName(netsignal.hasAutoName()),
    mNewIsAutoName(mOldIsAutoName) {
}

CmdNetSignalEdit::~CmdNetSignalEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdNetSignalEdit::setName(const CircuitIdentifier& name,
                               bool isAutoName) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
  mNewIsAutoName = isAutoName;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdNetSignalEdit::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdNetSignalEdit::performUndo() {
  mCircuit.setNetSignalName(mNetSignal, mOldName, mOldIsAutoName);  // can throw
}

void CmdNetSignalEdit::performRedo() {
  mCircuit.setNetSignalName(mNetSignal, mNewName, mNewIsAutoName);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
