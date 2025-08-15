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
#include "cmdschematicimageremove.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/project/schematic/items/si_image.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguardlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicImageRemove::CmdSchematicImageRemove(
    SI_Image& image, TransactionalDirectory& dir) noexcept
  : UndoCommand(tr("Remove Image from Schematic")),
    mImage(image),
    mDirectory(dir),
    mFileContent() {
}

CmdSchematicImageRemove::~CmdSchematicImageRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicImageRemove::performExecute() {
  // Check if this was the last image referencing the file. In this case, the
  // file will be removed too.
  bool fileReferencedByOtherImages = false;
  for (const SI_Image* img : mImage.getSchematic().getImages()) {
    if ((img != &mImage) &&
        (img->getImage()->getFileName() == mImage.getImage()->getFileName())) {
      fileReferencedByOtherImages = true;
      break;
    }
  }
  if (!fileReferencedByOtherImages) {
    mFileContent = mDirectory.readIfExists(*mImage.getImage()->getFileName());
  }

  performRedo();  // can throw
  return true;
}

void CmdSchematicImageRemove::performUndo() {
  ScopeGuardList sgl;
  if (!mFileContent.isNull()) {
    mDirectory.write(*mImage.getImage()->getFileName(),
                     mFileContent);  // can throw
    sgl.add(
        [this]() { mDirectory.removeFile(*mImage.getImage()->getFileName()); });
  }
  mImage.getSchematic().addImage(mImage);  // can throw
  sgl.dismiss();
}

void CmdSchematicImageRemove::performRedo() {
  ScopeGuardList sgl;
  if (!mFileContent.isNull()) {
    mDirectory.removeFile(*mImage.getImage()->getFileName());  // can throw
    sgl.add([this]() {
      mDirectory.write(*mImage.getImage()->getFileName(), mFileContent);
    });
  }
  mImage.getSchematic().removeImage(mImage);  // can throw
  sgl.dismiss();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
