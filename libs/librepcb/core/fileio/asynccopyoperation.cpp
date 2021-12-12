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
#include "asynccopyoperation.h"

#include "../exceptions.h"
#include "fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AsyncCopyOperation::AsyncCopyOperation(const FilePath& source,
                                       const FilePath& destination,
                                       QObject* parent) noexcept
  : QThread(parent), mSource(source), mDestination(destination) {
}

AsyncCopyOperation::~AsyncCopyOperation() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AsyncCopyOperation::run() noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!
  emit started();

  try {
    // Abort if destination already exists. Otherwise it would be deleted
    // during cleanup, which might not be intended.
    if (mDestination.isExistingFile() || mDestination.isExistingDir()) {
      throw LogicError(__FILE__, __LINE__,
                       tr("The file or directory \"%1\" exists already.")
                           .arg(mDestination.toNative()));
    }

    // First copy to a temporary directory and rename it afterwards to make
    // the operation "more atomic", i.e. avoiding half-copied destination in
    // in case of errors.
    emit progressStatus(tr("Removing temporary directory..."));
    FilePath tmpDst(mDestination.toStr() % "~");
    FileUtils::removeDirRecursively(tmpDst);  // can throw
    FileUtils::makePath(tmpDst);  // can throw

    // Get list of entries to copy
    emit progressStatus(tr("Looking for files to copy..."));
    QList<FilePath> files = FileUtils::getFilesInDirectory(
        mSource, QStringList(), true);  // can throw

    try {
      for (int i = 0; i < files.count(); ++i) {
        FilePath src = files.at(i);
        QString srcRelative = src.toRelative(mSource);
        FilePath dst = tmpDst.getPathTo(srcRelative);
        if (i % ((files.count() / 100) + 1) == 0) {
          emit progressStatus(
              tr("Copy file %1 of %2...").arg(i + 1).arg(files.count()));
          emit progressPercent((95 * (i + 1)) / files.count());
        }
        FileUtils::makePath(dst.getParentDir());  // can throw
        FileUtils::copyFile(src, dst);  // can throw
      }

      emit progressStatus(tr("Renaming temporary directory..."));
      emit progressPercent(98);
      FileUtils::move(tmpDst, mDestination);  // can throw

      emit progressStatus(tr("Successfully finished!"));
      emit progressPercent(100);
      emit succeeded();
    } catch (const Exception& e) {
      // clean up, but ignore failures to avoid misleading error messages
      QDir(mDestination.toStr()).removeRecursively();
      QDir(tmpDst.toStr()).removeRecursively();
      throw e;
    }
  } catch (const Exception& e) {
    emit progressStatus(tr("Failed to copy files:") % " " % e.getMsg());
    emit failed(e.getMsg());
  }

  emit finished();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
