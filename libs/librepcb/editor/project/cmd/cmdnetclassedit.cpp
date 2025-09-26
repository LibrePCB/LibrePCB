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

CmdNetClassEdit::CmdNetClassEdit(NetClass& netclass) noexcept
  : UndoCommand(tr("Edit Net Class")),
    mNetClass(netclass),
    mOldName(netclass.getName()),
    mNewName(mOldName),
    mOldDefaultTraceWidth(netclass.getDefaultTraceWidth()),
    mNewDefaultTraceWidth(mOldDefaultTraceWidth),
    mOldDefaultViaDrill(netclass.getDefaultViaDrill()),
    mNewDefaultViaDrill(mOldDefaultViaDrill) {
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

void CmdNetClassEdit::setDefaultTraceWidth(
    const std::optional<PositiveLength>& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDefaultTraceWidth = value;
}

void CmdNetClassEdit::setDefaultViaDrill(
    const std::optional<PositiveLength>& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDefaultViaDrill = value;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdNetClassEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewDefaultTraceWidth != mOldDefaultTraceWidth) return true;
  if (mNewDefaultViaDrill != mOldDefaultViaDrill) return true;
  return false;
}

void CmdNetClassEdit::performUndo() {
  mNetClass.getCircuit().setNetClassName(mNetClass, mOldName);  // can throw
  mNetClass.setDefaultTraceWidth(mOldDefaultTraceWidth);
  mNetClass.setDefaultViaDrill(mOldDefaultViaDrill);
}

void CmdNetClassEdit::performRedo() {
  mNetClass.getCircuit().setNetClassName(mNetClass, mNewName);  // can throw
  mNetClass.setDefaultTraceWidth(mNewDefaultTraceWidth);
  mNetClass.setDefaultViaDrill(mNewDefaultViaDrill);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
