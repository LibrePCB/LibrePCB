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
#include "smartfile.h"

#include "fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SmartFile::SmartFile(const FilePath& filepath, bool restore, bool readOnly,
                     bool create)
  : mFilePath(filepath),
    mTmpFilePath(filepath.toStr() % '~'),
    mOpenedFilePath(filepath),
    mIsRestored(restore),
    mIsReadOnly(readOnly),
    mIsCreated(create) {
  if (create) {
    Q_ASSERT(mIsRestored == false);
    Q_ASSERT(mIsReadOnly == false);

    // remove the files if they exists already
    if (mFilePath.isExistingFile()) {
      FileUtils::removeFile(mFilePath);
    }
    if (mTmpFilePath.isExistingFile()) {
      FileUtils::removeFile(mTmpFilePath);
    }
  } else {
    // decide if we open the original file (*.*) or the backup (*.*~)
    if ((mIsRestored) && (mTmpFilePath.isExistingFile())) {
      mOpenedFilePath = mTmpFilePath;
    }

    // check if the file exists
    if (!mOpenedFilePath.isExistingFile()) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString(tr("The file \"%1\" does not exist!"))
                             .arg(mOpenedFilePath.toNative()));
    }
  }
}

SmartFile::~SmartFile() noexcept {
  // remove temporary file, if required
  if ((!mIsRestored) && (!mIsReadOnly) && (mTmpFilePath.isExistingFile())) {
    try {
      FileUtils::removeFile(mTmpFilePath);
    } catch (const Exception& e) {
      qWarning() << "Could not remove temporary file:" << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SmartFile::removeFile(bool original) {
  if (mIsReadOnly) {
    throw LogicError(__FILE__, __LINE__, tr("Cannot remove read-only file!"));
  }

  FilePath filepath(original ? mFilePath : mTmpFilePath);
  if (filepath.isExistingFile()) {
    FileUtils::removeFile(filepath);
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const FilePath& SmartFile::prepareSaveAndReturnFilePath(bool toOriginal) {
  if (mIsReadOnly) {
    throw LogicError(__FILE__, __LINE__, tr("Cannot save read-only file!"));
  }

  return toOriginal ? mFilePath : mTmpFilePath;
}

void SmartFile::updateMembersAfterSaving(bool toOriginal) noexcept {
  if (toOriginal && mIsRestored) mIsRestored = false;

  if (toOriginal && mIsCreated) mIsCreated = false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
