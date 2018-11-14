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
#include "cmdlibraryelementedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdLibraryElementEdit::CmdLibraryElementEdit(LibraryElement& element,
                                             const QString&  text) noexcept
  : CmdLibraryBaseElementEdit(element, text),
    mElement(element),
    mOldCategories(element.getCategories()),
    mNewCategories(mOldCategories) {
}

CmdLibraryElementEdit::~CmdLibraryElementEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdLibraryElementEdit::setCategories(const QSet<Uuid>& uuids) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCategories = uuids;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdLibraryElementEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewCategories != mOldCategories) return true;
  return false;
}

void CmdLibraryElementEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mElement.setCategories(mOldCategories);
}

void CmdLibraryElementEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mElement.setCategories(mNewCategories);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
