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
#include "cmdpackageedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPackageEdit::CmdPackageEdit(Package& package) noexcept
  : CmdLibraryElementEdit(package, tr("Edit Package Properties")),
    mPackage(package),
    mOldAssemblyType(package.getAssemblyType(false)),
    mNewAssemblyType(mOldAssemblyType) {
}

CmdPackageEdit::~CmdPackageEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPackageEdit::setAssemblyType(Package::AssemblyType type) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAssemblyType = type;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageEdit::performExecute() {
  if (CmdLibraryElementEdit::performExecute()) return true;  // can throw
  if (mNewAssemblyType != mOldAssemblyType) return true;
  return false;
}

void CmdPackageEdit::performUndo() {
  CmdLibraryElementEdit::performUndo();  // can throw
  mPackage.setAssemblyType(mOldAssemblyType);
}

void CmdPackageEdit::performRedo() {
  CmdLibraryElementEdit::performRedo();  // can throw
  mPackage.setAssemblyType(mNewAssemblyType);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
