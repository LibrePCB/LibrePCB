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
#include "cmdassemblyvariantedit.h"

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

CmdAssemblyVariantEdit::CmdAssemblyVariantEdit(
    Circuit& circuit, std::shared_ptr<AssemblyVariant> av) noexcept
  : UndoCommand(tr("Edit assembly variant")),
    mCircuit(circuit),
    mAssemblyVariant(av),
    mOldName(av->getName()),
    mNewName(mOldName),
    mOldDescription(av->getDescription()),
    mNewDescription(mOldDescription) {
}

CmdAssemblyVariantEdit::~CmdAssemblyVariantEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdAssemblyVariantEdit::setName(const FileProofName& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = value;
}

void CmdAssemblyVariantEdit::setDescription(const QString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDescription = value;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAssemblyVariantEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewDescription != mOldDescription) return true;
  return false;
}

void CmdAssemblyVariantEdit::performUndo() {
  mCircuit.setAssemblyVariantName(mAssemblyVariant, mOldName);  // can throw
  mAssemblyVariant->setDescription(mOldDescription);
}

void CmdAssemblyVariantEdit::performRedo() {
  mCircuit.setAssemblyVariantName(mAssemblyVariant, mNewName);  // can throw
  mAssemblyVariant->setDescription(mNewDescription);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
