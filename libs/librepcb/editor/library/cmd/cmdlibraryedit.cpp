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
#include "cmdlibraryedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdLibraryEdit::CmdLibraryEdit(Library& library) noexcept
  : CmdLibraryBaseElementEdit(library, tr("Edit library metadata")),
    mLibrary(library),
    mOldUrl(library.getUrl()),
    mNewUrl(mOldUrl),
    mOldDependencies(library.getDependencies()),
    mNewDependencies(mOldDependencies),
    mOldIcon(library.getIcon()),
    mNewIcon(mOldIcon) {
}

CmdLibraryEdit::~CmdLibraryEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdLibraryEdit::setUrl(const QUrl& url) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewUrl = url;
}

void CmdLibraryEdit::setDependencies(const QSet<Uuid>& deps) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDependencies = deps;
}

void CmdLibraryEdit::setIcon(const QByteArray& png) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIcon = png;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdLibraryEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewUrl != mOldUrl) return true;
  if (mNewDependencies != mOldDependencies) return true;
  if (mNewIcon != mOldIcon) return true;
  return false;
}

void CmdLibraryEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mLibrary.setUrl(mOldUrl);
  mLibrary.setDependencies(mOldDependencies);
  mLibrary.setIcon(mOldIcon);
}

void CmdLibraryEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mLibrary.setUrl(mNewUrl);
  mLibrary.setDependencies(mNewDependencies);
  mLibrary.setIcon(mNewIcon);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
