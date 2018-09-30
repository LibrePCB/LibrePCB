/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "librarydownload.h"

#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/network/filedownload.h>
#include <librepcb/library/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace manager {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryDownload::LibraryDownload(const QUrl&     urlToZip,
                                 const FilePath& destDir) noexcept
  : QObject(nullptr),
    mDestDir(destDir),
    mTempDestDir(destDir.toStr() % ".tmp") {
  mFileDownload.reset(
      new FileDownload(urlToZip, FilePath(mDestDir.toStr() % ".zip")));
  mFileDownload->setZipExtractionDirectory(mTempDestDir);
  connect(mFileDownload.data(), &FileDownload::progressState, this,
          &LibraryDownload::progressState, Qt::QueuedConnection);
  connect(mFileDownload.data(), &FileDownload::progressPercent, this,
          &LibraryDownload::progressPercent, Qt::QueuedConnection);
  connect(mFileDownload.data(), &FileDownload::errored, this,
          &LibraryDownload::downloadErrored, Qt::QueuedConnection);
  connect(mFileDownload.data(), &FileDownload::aborted, this,
          &LibraryDownload::downloadAborted, Qt::QueuedConnection);
  connect(mFileDownload.data(), &FileDownload::succeeded, this,
          &LibraryDownload::downloadSucceeded, Qt::QueuedConnection);
  connect(this, &LibraryDownload::abortRequested, mFileDownload.data(),
          &FileDownload::abort, Qt::QueuedConnection);
}

LibraryDownload::~LibraryDownload() noexcept {
  abort();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LibraryDownload::setExpectedZipFileSize(qint64 bytes) noexcept {
  if (mFileDownload) {
    mFileDownload->setExpectedReplyContentSize(bytes);
  } else {
    qCritical() << "Calling this method after start() is not allowed!";
  }
}

void LibraryDownload::setExpectedChecksum(
    QCryptographicHash::Algorithm algorithm,
    const QByteArray&             checksum) noexcept {
  if (mFileDownload) {
    mFileDownload->setExpectedChecksum(algorithm, checksum);
  } else {
    qCritical() << "Calling this method after start() is not allowed!";
  }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void LibraryDownload::start() noexcept {
  if (!mFileDownload) {
    qCritical() << "Calling this method multiple times is not allowed!";
    return;
  }

  if (mTempDestDir.isExistingDir()) {
    try {
      FileUtils::removeDirRecursively(mTempDestDir);
    } catch (const Exception& e) {
      emit finished(false, e.getMsg());
      return;
    }
  }

  mFileDownload.take()
      ->start();  // release ownership of the FileDownload object!
}

void LibraryDownload::abort() noexcept {
  emit abortRequested();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryDownload::downloadErrored(const QString& errMsg) noexcept {
  emit LibraryDownload::finished(false, errMsg);
}

void LibraryDownload::downloadAborted() noexcept {
  emit LibraryDownload::finished(false, QString());
}

void LibraryDownload::downloadSucceeded() noexcept {
  // check if directory contains a library
  FilePath libDir = getPathToLibDir();
  if (!libDir.isValid()) {
    try {
      FileUtils::removeDirRecursively(mDestDir);
    } catch (...) {
    }  // clean up
    emit finished(
        false,
        tr("The downloaded ZIP file does not contain a LibrePCB library."));
    return;
  }

  // back-up existing library (if any)
  FilePath backupDir = FilePath(mDestDir.toStr() % ".backup");
  try {
    FileUtils::removeDirRecursively(backupDir);  // can throw
    if (mDestDir.isExistingDir())
      FileUtils::move(mDestDir, backupDir);  // can throw
  } catch (const Exception& e) {
    try {
      FileUtils::removeDirRecursively(backupDir);
    } catch (...) {
    }
    emit finished(false, e.getMsg());
    return;
  }

  // move downloaded directory to destination
  try {
    FileUtils::move(libDir, mDestDir);  // can throw
  } catch (const Exception& e) {
    try {
      FileUtils::removeDirRecursively(mDestDir);
      FileUtils::move(backupDir, mDestDir);
      FileUtils::removeDirRecursively(mTempDestDir);
    } catch (...) {
    }
    emit finished(false, e.getMsg());
    return;
  }

  // clean up
  try {
    FileUtils::removeDirRecursively(mTempDestDir);  // can throw
    FileUtils::removeDirRecursively(backupDir);     // can throw
  } catch (...) {
  }

  emit finished(true, QString());
}

FilePath LibraryDownload::getPathToLibDir() noexcept {
  if (library::Library::isValidElementDirectory<library::Library>(
          mTempDestDir)) {
    return mTempDestDir;
  }

  QStringList subdirs =
      QDir(mTempDestDir.toStr()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  if (subdirs.count() != 1) {
    return FilePath();
  }

  FilePath subdir = mTempDestDir.getPathTo(subdirs.first());
  if (library::Library::isValidElementDirectory<library::Library>(subdir)) {
    return subdir;
  } else {
    return FilePath();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb
