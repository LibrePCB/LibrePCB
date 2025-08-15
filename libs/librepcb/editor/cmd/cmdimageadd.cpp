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
#include "cmdimageadd.h"

#include <librepcb/core/fileio/transactionaldirectory.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdImageAdd::CmdImageAdd(ImageList& list, TransactionalDirectory& dir,
                         std::shared_ptr<Image> image,
                         const QByteArray& fileContent) noexcept
  : UndoCommand(tr("Add Image")),
    mList(list),
    mDirectory(dir),
    mImage(image),
    mFileContent(fileContent) {
}

CmdImageAdd::~CmdImageAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdImageAdd::performExecute() {
  // Sanity check that the file doesn't exist yet if it will be written.
  if ((!mFileContent.isNull()) &&
      mDirectory.fileExists(*mImage->getFileName())) {
    throw LogicError(__FILE__, __LINE__,
                     QString("File '%1' exists already. This should not "
                             "happen, please open a bug report.")
                         .arg(*mImage->getFileName()));
  } else if (mFileContent.isNull() &&
             (!mDirectory.fileExists(*mImage->getFileName()))) {
    throw LogicError(__FILE__, __LINE__,
                     QString("File '%1' does not exist yet. This should not "
                             "happen, please open a bug report.")
                         .arg(*mImage->getFileName()));
  }

  performRedo();  // can throw
  return true;
}

void CmdImageAdd::performUndo() {
  if (!mFileContent.isNull()) {
    mDirectory.removeFile(*mImage->getFileName());  // can throw
  }
  mList.remove(mImage.get());
}

void CmdImageAdd::performRedo() {
  if (!mFileContent.isNull()) {
    mDirectory.write(*mImage->getFileName(), mFileContent);  // can throw
  }
  mList.append(mImage);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
