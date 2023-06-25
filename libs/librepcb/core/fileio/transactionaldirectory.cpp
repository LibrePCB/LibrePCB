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
#include "transactionaldirectory.h"

#include "transactionalfilesystem.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TransactionalDirectory::TransactionalDirectory(QObject* parent)
  : FileSystem(parent),
    // Open file system in read-only mode to avoid creating ".lock" file!
    mFileSystem(TransactionalFileSystem::openRO(FilePath::getRandomTempPath())),
    mPath() {
}

TransactionalDirectory::TransactionalDirectory(
    std::shared_ptr<TransactionalFileSystem> fs, const QString& dir,
    QObject* parent) noexcept
  : FileSystem(parent),
    mFileSystem(fs),
    mPath(TransactionalFileSystem::cleanPath(dir)) {
  Q_ASSERT(mFileSystem);
}

TransactionalDirectory::TransactionalDirectory(TransactionalDirectory& other,
                                               const QString& subdir,
                                               QObject* parent) noexcept
  : FileSystem(parent),
    mFileSystem(other.mFileSystem),
    mPath(TransactionalFileSystem::cleanPath(other.mPath % "/" % subdir)) {
}

TransactionalDirectory::~TransactionalDirectory() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool TransactionalDirectory::isWritable() const noexcept {
  return mFileSystem->isWritable();
}

bool TransactionalDirectory::isRestoredFromAutosave() const noexcept {
  return mFileSystem->isRestoredFromAutosave();
}

/*******************************************************************************
 *  Inherited from FileSystem
 ******************************************************************************/

FilePath TransactionalDirectory::getAbsPath(const QString& path) const
    noexcept {
  return mFileSystem->getAbsPath(mPath % "/" % path);
}

QStringList TransactionalDirectory::getDirs(const QString& path) const
    noexcept {
  return mFileSystem->getDirs(mPath % "/" % path);
}

QStringList TransactionalDirectory::getFiles(const QString& path) const
    noexcept {
  return mFileSystem->getFiles(mPath % "/" % path);
}

bool TransactionalDirectory::fileExists(const QString& path) const noexcept {
  return mFileSystem->fileExists(mPath % "/" % path);
}

QByteArray TransactionalDirectory::read(const QString& path) const {
  return mFileSystem->read(mPath % "/" % path);
}

QByteArray TransactionalDirectory::readIfExists(const QString& path) const {
  return mFileSystem->readIfExists(mPath % "/" % path);
}

void TransactionalDirectory::write(const QString& path,
                                   const QByteArray& content) {
  mFileSystem->write(mPath % "/" % path, content);
}

void TransactionalDirectory::renameFile(const QString& src,
                                        const QString& dst) {
  mFileSystem->renameFile(mPath % "/" % src, mPath % "/" % dst);
}

void TransactionalDirectory::removeFile(const QString& path) {
  mFileSystem->removeFile(mPath % "/" % path);
}

void TransactionalDirectory::removeDirRecursively(const QString& path) {
  mFileSystem->removeDirRecursively(mPath % "/" % path);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void TransactionalDirectory::copyTo(TransactionalDirectory& dest) const {
  // copy files to destination
  QString srcDir = mPath.isEmpty() ? mPath : mPath % "/";
  QString dstDir = dest.mPath.isEmpty() ? dest.mPath : dest.mPath % "/";
  copyDirRecursively(*mFileSystem, srcDir, *dest.mFileSystem,
                     dstDir);  // can throw
}

void TransactionalDirectory::saveTo(TransactionalDirectory& dest) {
  // copy files to destination
  copyTo(dest);  // can throw

  // update members
  mFileSystem = dest.mFileSystem;
  mPath = dest.mPath;
}

void TransactionalDirectory::moveTo(TransactionalDirectory& dest) {
  // copy files to destination
  copyTo(dest);  // can throw

  // remove files from source
  mFileSystem->removeDirRecursively(mPath);  // can throw

  // update members
  mFileSystem = dest.mFileSystem;
  mPath = dest.mPath;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void TransactionalDirectory::copyDirRecursively(TransactionalFileSystem& srcFs,
                                                const QString& srcDir,
                                                TransactionalFileSystem& dstFs,
                                                const QString& dstDir) {
  Q_ASSERT(srcDir.isEmpty() || srcDir.endsWith('/'));
  Q_ASSERT(dstDir.isEmpty() || dstDir.endsWith('/'));

  // copy files
  foreach (const QString& file, srcFs.getFiles(srcDir)) {
    dstFs.write(dstDir % file, srcFs.read(srcDir % file));
  }

  // copy directories
  foreach (const QString& dir, srcFs.getDirs(srcDir)) {
    copyDirRecursively(srcFs, srcDir % dir % "/", dstFs, dstDir % dir % "/");
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
