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
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdLibraryElementEdit::CmdLibraryElementEdit(LibraryElement& element,
                                             const QString& text) noexcept
  : CmdLibraryBaseElementEdit(element, text),
    mElement(element),
    mOldGeneratedBy(element.getGeneratedBy()),
    mNewGeneratedBy(mOldGeneratedBy),
    mOldCategories(element.getCategories()),
    mNewCategories(mOldCategories),
    mOldResources(element.getResources()),
    mNewResources(mOldResources) {
}

CmdLibraryElementEdit::~CmdLibraryElementEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdLibraryElementEdit::setGeneratedBy(
    const QString& generatedBy) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewGeneratedBy = generatedBy;
}

void CmdLibraryElementEdit::setCategories(const QSet<Uuid>& uuids) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCategories = uuids;
}

void CmdLibraryElementEdit::setResources(
    const ResourceList& resources) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewResources = resources;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdLibraryElementEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewGeneratedBy != mOldGeneratedBy) return true;
  if (mNewCategories != mOldCategories) return true;
  if (mNewResources != mOldResources) return true;
  return false;
}

void CmdLibraryElementEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mElement.setGeneratedBy(mOldGeneratedBy);
  mElement.setCategories(mOldCategories);
  mElement.setResources(mOldResources);
}

void CmdLibraryElementEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mElement.setGeneratedBy(mNewGeneratedBy);
  mElement.setCategories(mNewCategories);
  mElement.setResources(mNewResources);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
