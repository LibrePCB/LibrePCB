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
  : CmdLibraryElementEdit(element, tr("Reload Symbol")),
    mElement(element),
    mOldFiles(mElement.getDirectory().getFileSystem()->saveState()),
    mNewFiles(),
    mOldPins(mElement.getPins()),
    mNewPins(mOldPins),
    mOldPolygons(mElement.getPolygons()),
    mNewPolygons(mOldPolygons),
    mOldCircles(mElement.getCircles()),
    mNewCircles(mOldCircles),
    mOldTexts(mElement.getTexts()),
    mNewTexts(mOldTexts) {
}

CmdSymbolReload::~CmdSymbolReload() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSymbolReload::performExecute() {
  // First of all, load the new element in read-only mode to verify it is valid.
  auto fs = TransactionalFileSystem::openRO(
      mElement.getDirectory().getAbsPath());  // can throw
  auto newElement =
      Symbol::open(std::make_unique<TransactionalDirectory>(fs));  // can throw

  // Now discard any pending file I/O of the loaded element.
  if (mElement.getDirectory().getFileSystem()->getAbsPath() !=
      mElement.getDirectory().getAbsPath()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mElement.getDirectory().getFileSystem()->discardChanges();

  // Then copy over everything from the newly opened element.
  setNames(newElement->getNames());
  setDescriptions(newElement->getDescriptions());
  setKeywords(newElement->getKeywords());
  setVersion(newElement->getVersion());
  setAuthor(newElement->getAuthor());
  setCreated(newElement->getCreated());
  setDeprecated(newElement->isDeprecated());
  setGeneratedBy(newElement->getGeneratedBy());
  setCategories(newElement->getCategories());
  setResources(newElement->getResources());
  mNewPins = newElement->getPins();
  mNewPolygons = newElement->getPolygons();
  mNewCircles = newElement->getCircles();
  mNewTexts = newElement->getTexts();

  // And apply the modifications.
  if (CmdLibraryElementEdit::performExecute()) return true;  // can throw
  if (mNewPins != mOldPins) return true;
  if (mNewPolygons != mOldPolygons) return true;
  if (mNewCircles != mOldCircles) return true;
  if (mNewTexts != mOldTexts) return true;
  return false;
}

void CmdSymbolReload::performUndo() {
  CmdLibraryElementEdit::performUndo();  // can throw
  mElement.getDirectory().getFileSystem()->restoreState(mOldFiles);
  mElement.getPins() = mOldPins;
  mElement.getPolygons() = mOldPolygons;
  mElement.getCircles() = mOldCircles;
  mElement.getTexts() = mOldTexts;
}

void CmdSymbolReload::performRedo() {
  CmdLibraryElementEdit::performRedo();  // can throw
  mElement.getDirectory().getFileSystem()->restoreState(mNewFiles);
  mElement.getPins() = mNewPins;
  mElement.getPolygons() = mNewPolygons;
  mElement.getCircles() = mNewCircles;
  mElement.getTexts() = mNewTexts;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
