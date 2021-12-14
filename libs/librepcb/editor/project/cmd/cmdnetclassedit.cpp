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
#include "cmdnetclassedit.h"

#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netclass.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdNetClassEdit::CmdNetClassEdit(Circuit& circuit, NetClass& netclass) noexcept
  : UndoCommand(tr("Edit netclass")),
    mCircuit(circuit),
    mNetClass(netclass),
    mOldName(netclass.getName()),
    mNewName(mOldName) {
}

CmdNetClassEdit::~CmdNetClassEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdNetClassEdit::setName(const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdNetClassEdit::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdNetClassEdit::performUndo() {
  mCircuit.setNetClassName(mNetClass, mOldName);  // can throw
}

void CmdNetClassEdit::performRedo() {
  mCircuit.setNetClassName(mNetClass, mNewName);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
