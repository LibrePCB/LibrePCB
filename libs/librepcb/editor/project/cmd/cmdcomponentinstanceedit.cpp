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
#include "cmdcomponentinstanceedit.h"

#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentInstanceEdit::CmdComponentInstanceEdit(
    Circuit& circuit, ComponentInstance& cmp) noexcept
  : UndoCommand(tr("Edit Component")),
    mCircuit(circuit),
    mComponentInstance(cmp),
    mOldName(cmp.getName()),
    mNewName(mOldName),
    mOldValue(cmp.getValue()),
    mNewValue(mOldValue),
    mOldAttributes(cmp.getAttributes()),
    mNewAttributes(mOldAttributes),
    mOldAssemblyOptions(cmp.getAssemblyOptions()),
    mNewAssemblyOptions(mOldAssemblyOptions) {
}

CmdComponentInstanceEdit::~CmdComponentInstanceEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdComponentInstanceEdit::setName(const CircuitIdentifier& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

void CmdComponentInstanceEdit::setValue(const QString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewValue = value;
}

void CmdComponentInstanceEdit::setAttributes(
    const AttributeList& attributes) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAttributes = attributes;
}

void CmdComponentInstanceEdit::setAssemblyOptions(
    const ComponentAssemblyOptionList& options) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAssemblyOptions = options;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentInstanceEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewValue != mOldValue) return true;
  if (mNewAttributes != mOldAttributes) return true;
  if (mNewAssemblyOptions != mOldAssemblyOptions) return true;
  return false;
}

void CmdComponentInstanceEdit::performUndo() {
  mCircuit.setComponentInstanceName(mComponentInstance, mOldName);  // can throw
  mComponentInstance.setValue(mOldValue);
  mComponentInstance.setAttributes(mOldAttributes);
  mComponentInstance.setAssemblyOptions(mOldAssemblyOptions);
}

void CmdComponentInstanceEdit::performRedo() {
  mCircuit.setComponentInstanceName(mComponentInstance, mNewName);  // can throw
  mComponentInstance.setValue(mNewValue);
  mComponentInstance.setAttributes(mNewAttributes);
  mComponentInstance.setAssemblyOptions(mNewAssemblyOptions);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
