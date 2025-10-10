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
#include "cmdsymbolreload.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSymbolReload::CmdSymbolReload(Symbol& element) noexcept
  : CmdLibraryElementEdit(element, tr("Reload Symbol")), mElement(element) {
}

CmdSymbolReload::~CmdSymbolReload() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSymbolReload::performExecute() {
  // First of all, load the new symbol in read-only mode to verify it is valid.
  auto fs = TransactionalFileSystem::openRO(
      mElement.getDirectory().getAbsPath());  // can throw
  auto sym =
      Symbol::open(std::make_unique<TransactionalDirectory>(fs));  // can throw

  // Now discard any pending file I/O of the loaded symbol.
  // TODO: This needs to be undone when undoing this command!
  if (mElement.getDirectory().getFileSystem()->getAbsPath() !=
      mElement.getDirectory().getAbsPath()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mElement.getDirectory().getFileSystem()->discardChanges();

  // Then copy over everything from the newly opened symbol.
  setNames(sym->getNames());
  setDescriptions(sym->getDescriptions());
  setKeywords(sym->getKeywords());
  setVersion(sym->getVersion());
  setAuthor(sym->getAuthor());
  setDeprecated(sym->isDeprecated());
  setCategories(sym->getCategories());
  setResources(sym->getResources());
  return CmdLibraryBaseElementEdit::performExecute();  // can throw
}

void CmdSymbolReload::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
}

void CmdSymbolReload::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
