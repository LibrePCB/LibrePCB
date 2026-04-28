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
#include "filedownload.h"

#include "../exceptions.h"
#include "../fileio/fileutils.h"
#include "../fileio/ziparchive.h"
#include "../utils/scopeguardlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileDownload::FileDownload(const QUrl& url, const FilePath& dest,
                           std::shared_ptr<QSemaphore> semaphore) noexcept
  : NetworkRequestBase(url),
    mSemaphore(semaphore),
    mDestination(dest),
    mHash(),
    mExpectedChecksum(),
    mExtractZipToDir() {
  Q_ASSERT(mSemaphore);
}

FileDownload::~FileDownload() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void FileDownload::setExpectedChecksum(QCryptographicHash::Algorithm algorithm,
                                       const QByteArray& checksum) noexcept {
  Q_ASSERT(!mStarted);
  mHash.reset(new QCryptographicHash(algorithm));
  mExpectedChecksum = checksum;
}

void FileDownload::setZipExtractionDirectory(
    const FilePath& dir,
    std::function<FilePath(const FilePath&)> discoveryCallback) noexcept {
  Q_ASSERT(!mStarted);
  mExtractZipToDir = dir;
  mZipDiscoveryCallback = discoveryCallback;
}

void FileDownload::setZipCleanupCallback(
    std::function<void(const FilePath&)> cb) noexcept {
  Q_ASSERT(!mStarted);
  mZipCleanupCallback = cb;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileDownload::prepareRequest() {
  // Limit number of parallel file access threads.
  mSemaphore->acquire();
  QSemaphoreReleaser releaser(*mSemaphore);

  // Create destination directory.
  FileUtils::makePath(mDestination.getParentDir());  // can throw

  // Open temporary destination file.
  mFile.reset(new QSaveFile(mDestination.toStr(), this));
  if (!mFile->open(QIODevice::WriteOnly)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Could not open file \"%1\": %2")
                           .arg(mDestination.toNative(), mFile->errorString()));
  }
}

void FileDownload::finalizeRequest() {
  // Verify checksum of downloaded file.
  if (mHash) {
    emit progressState(tr("Verify checksum..."));
    const QString result = mHash->result().toHex();
    const QString expected = mExpectedChecksum.toHex();
    if (result != expected) {
      qDebug().nospace() << "Expected checksum " << expected << " but got "
                         << result << ".";
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Checksum verification of downloaded file failed!"));
    } else {
      qDebug() << "Checksum verification of downloaded file was successful.";
    }
  }

  // Limit number of parallel file access / unzip threads.
  mSemaphore->acquire();
  QSemaphoreReleaser releaser(*mSemaphore);

  // Save to destination file.
  emit progressState(tr("Write file..."));
  if (!mFile->commit()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Error while writing file \"%1\": %2")
                           .arg(mDestination.toNative(), mFile->errorString()));
  }

  // If the downloaded file is not a ZIP, we are done now.
  if (!mExtractZipToDir.isValid()) {
    return;
  }

  // Clean up temporary files and folders.
  emit progressState(tr("Remove temporary files..."));
  ScopeGuardList sgl;
  sgl.add([fp = mDestination.toStr()]() { QFile::remove(fp); });
  const FilePath tmpDirFp(mExtractZipToDir.toStr() % ".tmp");
  FileUtils::removeDirRecursively(tmpDirFp);  // can throw
  sgl.add([fp = tmpDirFp.toStr()]() { QDir(fp).removeRecursively(); });
  const FilePath backupDirFp(mExtractZipToDir.toStr() % ".backup");
  FileUtils::removeDirRecursively(backupDirFp);  // can throw
  sgl.add([fp = backupDirFp.toStr()]() { QDir(fp).removeRecursively(); });

  // Extract ZIP to temporary directory.
  emit progressState(tr("Extract ZIP..."));
  ZipArchive zip(mDestination);  // can throw
  zip.extractTo(tmpDirFp);  // can throw

  // Find the directory of interest in the temporary directory.
  const FilePath srcDirFp = mZipDiscoveryCallback
      ? mZipDiscoveryCallback(tmpDirFp)  // can throw
      : tmpDirFp;
  Q_ASSERT(srcDirFp.isValid() && srcDirFp.isLocatedInDir(tmpDirFp));

  // Create backup of destination directory. This is better than removing
  // the destination directory recursively, as this would not be atomic.
  if (mExtractZipToDir.isExistingDir()) {
    FileUtils::move(mExtractZipToDir, backupDirFp);  // can throw
  }

  // Move downloaded directory to destination.
  // If this fails, try to restore the backup.
  try {
    FileUtils::move(srcDirFp, mExtractZipToDir);  // can throw
  } catch (const Exception& e) {
    QDir().rename(backupDirFp.toStr(), mExtractZipToDir.toStr());
    return;  // Do not call cleanup callback!
  }

  // Call cleanup callback.
  if (mZipCleanupCallback) {
    mZipCleanupCallback(mExtractZipToDir);
  }
}

void FileDownload::emitSuccessfullyFinishedSignals(
    QString contentType) noexcept {
  Q_UNUSED(contentType);
  emit fileDownloaded(mDestination);
  if (mExtractZipToDir.isValid()) {
    emit zipFileExtracted(mExtractZipToDir);
  }
}

void FileDownload::fetchNewData(QIODevice& device) noexcept {
  const QByteArray data = device.readAll();
  mFile->write(data);
  if (mHash) {
    mHash->addData(data);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
