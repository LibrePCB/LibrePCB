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
#include "cmdlibrarycategoryedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdLibraryCategoryEdit::CmdLibraryCategoryEdit(
    LibraryCategory& category) noexcept
  : CmdLibraryBaseElementEdit(category, tr("Edit category metadata")),
    mCategory(category),
    mOldParentUuid(category.getParentUuid()),
    mNewParentUuid(mOldParentUuid) {
}

CmdLibraryCategoryEdit::~CmdLibraryCategoryEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdLibraryCategoryEdit::setParentUuid(
    const tl::optional<Uuid>& parentUuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewParentUuid = parentUuid;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdLibraryCategoryEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewParentUuid != mOldParentUuid) return true;
  return false;
}

void CmdLibraryCategoryEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mCategory.setParentUuid(mOldParentUuid);
}

void CmdLibraryCategoryEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mCategory.setParentUuid(mNewParentUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
