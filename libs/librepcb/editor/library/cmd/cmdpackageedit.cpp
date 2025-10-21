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
    mOldAlternativeNames(package.getAlternativeNames()),
    mNewAlternativeNames(mOldAlternativeNames),
    mOldAssemblyType(package.getAssemblyType(false)),
    mNewAssemblyType(mOldAssemblyType),
    mOldMinCopperClearance(package.getMinCopperClearance()),
    mNewMinCopperClearance(mOldMinCopperClearance) {
}

CmdPackageEdit::~CmdPackageEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPackageEdit::setAlternativeNames(
    const QList<Package::AlternativeName>& names) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAlternativeNames = names;
}

void CmdPackageEdit::setAssemblyType(Package::AssemblyType type) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAssemblyType = type;
}

void CmdPackageEdit::setMinCopperClearance(const UnsignedLength& clr) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinCopperClearance = clr;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageEdit::performExecute() {
  if (CmdLibraryElementEdit::performExecute()) return true;  // can throw
  if (mNewAlternativeNames != mOldAlternativeNames) return true;
  if (mNewAssemblyType != mOldAssemblyType) return true;
  if (mNewMinCopperClearance != mOldMinCopperClearance) return true;
  return false;
}

void CmdPackageEdit::performUndo() {
  CmdLibraryElementEdit::performUndo();  // can throw
  mPackage.setAlternativeNames(mOldAlternativeNames);
  mPackage.setAssemblyType(mOldAssemblyType);
  mPackage.setMinCopperClearance(mOldMinCopperClearance);
}

void CmdPackageEdit::performRedo() {
  CmdLibraryElementEdit::performRedo();  // can throw
  mPackage.setAlternativeNames(mNewAlternativeNames);
  mPackage.setAssemblyType(mNewAssemblyType);
  mPackage.setMinCopperClearance(mNewMinCopperClearance);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
