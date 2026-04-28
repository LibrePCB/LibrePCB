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
#include "librarydownload.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/network/filedownload.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryDownload::LibraryDownload(const QUrl& urlToZip, const FilePath& destDir,
                                 std::shared_ptr<QSemaphore> semaphore) noexcept
  : QObject(nullptr) {
  mFileDownload.reset(new FileDownload(
      urlToZip, FilePath(destDir.toStr() % ".zip"), semaphore));
  mFileDownload->setZipExtractionDirectory(destDir,
                                           &LibraryDownload::findLibraryInZip);
  connect(mFileDownload.get(), &FileDownload::progressState, this,
          &LibraryDownload::progressState, Qt::QueuedConnection);
  connect(mFileDownload.get(), &FileDownload::progressPercent, this,
          &LibraryDownload::progressPercent, Qt::QueuedConnection);
  connect(mFileDownload.get(), &FileDownload::errored, this,
          &LibraryDownload::downloadErrored, Qt::QueuedConnection);
  connect(mFileDownload.get(), &FileDownload::aborted, this,
          &LibraryDownload::downloadAborted, Qt::QueuedConnection);
  connect(mFileDownload.get(), &FileDownload::succeeded, this,
          &LibraryDownload::downloadSucceeded, Qt::QueuedConnection);
  connect(this, &LibraryDownload::abortRequested, mFileDownload.get(),
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
    qCritical() << "Calling LibraryDownload::setExpectedZipFileSize() after "
                   "start() is not allowed!";
  }
}

void LibraryDownload::setExpectedChecksum(
    QCryptographicHash::Algorithm algorithm,
    const QByteArray& checksum) noexcept {
  if (mFileDownload) {
    mFileDownload->setExpectedChecksum(algorithm, checksum);
  } else {
    qCritical() << "Calling LibraryDownload::setExpectedChecksum() after "
                   "start() is not allowed!";
  }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void LibraryDownload::start() noexcept {
  if (!mFileDownload) {
    qCritical()
        << "Calling LibraryDownload::start() multiple times is not allowed!";
    return;
  }

  // Release ownership of the FileDownload object because it will be deleted by
  // itself after the download finished!
  mFileDownload.release()->start();
}

void LibraryDownload::abort() noexcept {
  emit abortRequested();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryDownload::downloadErrored(const QString& errMsg) noexcept {
  emit finished(false, errMsg);
}

void LibraryDownload::downloadAborted() noexcept {
  emit finished(false, QString());
}

void LibraryDownload::downloadSucceeded() noexcept {
  emit finished(true, QString());
}

FilePath LibraryDownload::findLibraryInZip(const FilePath& root) {
  if (Library::isValidElementDirectory<Library>(root)) {
    return root;
  }

  const QStringList subdirs =
      QDir(root.toStr()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  if (subdirs.count() == 1) {
    const FilePath subdir = root.getPathTo(subdirs.first());
    if (Library::isValidElementDirectory<Library>(subdir)) {
      return subdir;
    }
  }

  throw RuntimeError(
      __FILE__, __LINE__,
      tr("The downloaded ZIP file does not contain a LibrePCB library."));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
