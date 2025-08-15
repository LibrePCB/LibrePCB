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
#include "cmdimageremove.h"

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

CmdImageRemove::CmdImageRemove(ImageList& list, TransactionalDirectory& dir,
                               std::shared_ptr<Image> image) noexcept
  : UndoCommand(tr("Remove Image")),
    mList(list),
    mDirectory(dir),
    mImage(image),
    mFileContent(),
    mIndex(-1) {
}

CmdImageRemove::~CmdImageRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdImageRemove::performExecute() {
  // Check if this was the last image referencing the file. In this case, the
  // file will be removed too.
  bool fileReferencedByOtherImages = false;
  for (const std::shared_ptr<Image>& img : mList.values()) {
    if ((img != mImage) && (img->getFileName() == mImage->getFileName())) {
      fileReferencedByOtherImages = true;
      break;
    }
  }
  if (!fileReferencedByOtherImages) {
    mFileContent = mDirectory.readIfExists(*mImage->getFileName());
  }

  // Memorize current image index.
  mIndex = mList.indexOf(mImage.get());
  if (mIndex < 0) throw LogicError(__FILE__, __LINE__, "Element not in list.");

  performRedo();  // can throw
  return true;
}

void CmdImageRemove::performUndo() {
  if (!mFileContent.isNull()) {
    mDirectory.write(*mImage->getFileName(), mFileContent);  // can throw
  }
  mList.insert(mIndex, mImage);
}

void CmdImageRemove::performRedo() {
  if (!mFileContent.isNull()) {
    mDirectory.removeFile(*mImage->getFileName());  // can throw
  }
  auto item = mList.take(mIndex);
  Q_ASSERT(item == mImage);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
