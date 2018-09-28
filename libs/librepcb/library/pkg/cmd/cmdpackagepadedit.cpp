/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "cmdpackagepadedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPackagePadEdit::CmdPackagePadEdit(PackagePad& pad) noexcept
  : UndoCommand(tr("Edit package pad")),
    mPad(pad),
    mOldName(pad.getName()),
    mNewName(mOldName) {
}

CmdPackagePadEdit::~CmdPackagePadEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPackagePadEdit::setName(const CircuitIdentifier& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackagePadEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  return false;
}

void CmdPackagePadEdit::performUndo() {
  mPad.setName(mOldName);
}

void CmdPackagePadEdit::performRedo() {
  mPad.setName(mNewName);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
