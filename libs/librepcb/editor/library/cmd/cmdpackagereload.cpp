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
#include "cmdpackagereload.h"

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

CmdPackageReload::CmdPackageReload(Package& element) noexcept
  : CmdPackageEdit(element),
    mElement(element),
    mOldFiles(mElement.getDirectory().getFileSystem()->saveState()),
    mNewFiles(),
    mOldPads(mElement.getPads()),
    mNewPads(mOldPads),
    mOldModels(mElement.getModels()),
    mNewModels(mOldModels),
    mOldFootprints(mElement.getFootprints()),
    mNewFootprints(mOldFootprints) {
  mText = tr("Reload Package");
}

CmdPackageReload::~CmdPackageReload() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageReload::performExecute() {
  // First of all, load the new element in read-only mode to verify it is valid.
  auto fs = TransactionalFileSystem::openRO(
      mElement.getDirectory().getAbsPath());  // can throw
  auto newElement =
      Package::open(std::make_unique<TransactionalDirectory>(fs));  // can throw

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
  setAlternativeNames(newElement->getAlternativeNames());
  setAssemblyType(newElement->getAssemblyType(false));
  mNewPads = newElement->getPads();
  mNewModels = newElement->getModels();
  mNewFootprints = newElement->getFootprints();

  // And apply the modifications.
  if (CmdPackageEdit::performExecute()) return true;  // can throw
  if (mNewPads != mOldPads) return true;
  if (mNewModels != mOldModels) return true;
  if (mNewFootprints != mOldFootprints) return true;
  return false;
}

void CmdPackageReload::performUndo() {
  CmdPackageEdit::performUndo();  // can throw
  mElement.getDirectory().getFileSystem()->restoreState(mOldFiles);
  mElement.getPads() = mOldPads;
  mElement.getModels() = mOldModels;
  mElement.getFootprints() = mOldFootprints;
}

void CmdPackageReload::performRedo() {
  CmdPackageEdit::performRedo();  // can throw
  mElement.getDirectory().getFileSystem()->restoreState(mNewFiles);
  mElement.getPads() = mNewPads;
  mElement.getModels() = mNewModels;
  mElement.getFootprints() = mNewFootprints;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
