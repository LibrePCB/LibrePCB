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
#include "cmdschematicimageadd.h"

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

CmdSchematicImageAdd::CmdSchematicImageAdd(
    SI_Image& image, TransactionalDirectory& dir,
    const QByteArray& fileContent) noexcept
  : UndoCommand(tr("Add Image to Schematic")),
    mImage(image),
    mDirectory(dir),
    mFileContent(fileContent) {
}

CmdSchematicImageAdd::~CmdSchematicImageAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicImageAdd::performExecute() {
  // Sanity check that the file doesn't exist yet if it will be written.
  if ((!mFileContent.isNull()) &&
      mDirectory.fileExists(*mImage.getImage()->getFileName())) {
    throw LogicError(__FILE__, __LINE__,
                     QString("File '%1' exists already. This should not "
                             "happen, please open a bug report.")
                         .arg(*mImage.getImage()->getFileName()));
  } else if (mFileContent.isNull() &&
             (!mDirectory.fileExists(*mImage.getImage()->getFileName()))) {
    throw LogicError(__FILE__, __LINE__,
                     QString("File '%1' does not exist yet. This should not "
                             "happen, please open a bug report.")
                         .arg(*mImage.getImage()->getFileName()));
  }

  performRedo();  // can throw
  return true;
}

void CmdSchematicImageAdd::performUndo() {
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

void CmdSchematicImageAdd::performRedo() {
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
