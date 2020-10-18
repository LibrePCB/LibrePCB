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
#include "transactionalfilesystem.h"

#include "../toolbox.h"
#include "fileutils.h"
#include "sexpression.h"

#ifdef SYSTEM_QUAZIP
#include <quazip5/quazip.h>
#include <quazip5/quazipdir.h>
#include <quazip5/quazipfile.h>
#else
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>
#endif

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TransactionalFileSystem::TransactionalFileSystem(
    const FilePath& filepath, bool writable, RestoreCallback restoreCallback,
    QObject* parent)
  : FileSystem(parent),
    mFilePath(filepath),
    mIsWritable(writable),
    mLock(filepath),
    mRestoredFromAutosave(false) {
  // Load the backup if there is one (i.e. last save operation has failed).
  FilePath backupFile = mFilePath.getPathTo(".backup/backup.lp");
  if (backupFile.isExistingFile()) {
    qDebug() << "Restoring file system from backup:" << backupFile.toNative();
    loadDiff(backupFile);  // can throw
  }

  // Lock directory if the file system is opened in R/W mode.
  if (mIsWritable) {
    FileUtils::makePath(mFilePath);  // can throw
    mLock.tryLock();  // can throw
  }

  // If there is an autosave backup, load it according the restore mode.
  FilePath autosaveFile = mFilePath.getPathTo(".autosave/autosave.lp");
  if (autosaveFile.isExistingFile()) {
    if (restoreCallback && restoreCallback(mFilePath)) {  // can throw
      qDebug() << "Restoring file system from autosave backup:"
               << autosaveFile.toNative();
      loadDiff(autosaveFile);  // can throw
      mRestoredFromAutosave = true;
    }
  }
}

TransactionalFileSystem::~TransactionalFileSystem() noexcept {
  // Remove autosave directory as it is not needed in case the file system
  // was gracefully closed. We only need it if the application has crashed.
  // But if the file system is opened in read-only mode, or if an autosave was
  // restored but not saved in the meantime, do NOT remove the autosave
  // directory!
  if (mIsWritable && (!mRestoredFromAutosave)) {
    try {
      removeDiff("autosave");  // can throw
    } catch (const Exception& e) {
      qWarning() << "Could not remove autosave directory:" << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  Inherited from FileSystem
 ******************************************************************************/

FilePath TransactionalFileSystem::getAbsPath(const QString& path) const
    noexcept {
  return mFilePath.getPathTo(cleanPath(path));
}

QStringList TransactionalFileSystem::getDirs(const QString& path) const
    noexcept {
  QSet<QString> dirnames;
  QString dirpath = cleanPath(path);
  if (!dirpath.isEmpty()) dirpath.append("/");

  // add directories from file system, if not removed
  QDir dir(mFilePath.getPathTo(path).toStr());
  foreach (const QString& dirname,
           dir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)) {
    if (!isRemoved(dirpath % dirname % "/")) {
      dirnames.insert(dirname);
    }
  }

  // add directories of new files
  foreach (const QString& filepath, mModifiedFiles.keys()) {
    if (filepath.startsWith(dirpath)) {
      QStringList relpath = filepath.mid(dirpath.length()).split('/');
      if (relpath.count() > 1) {
        dirnames.insert(relpath.first());
      }
    }
  }

  return dirnames.values();
}

QStringList TransactionalFileSystem::getFiles(const QString& path) const
    noexcept {
  QSet<QString> filenames;
  QString dirpath = cleanPath(path);
  if (!dirpath.isEmpty()) dirpath.append("/");

  // add files from file system, if not removed
  QDir dir(mFilePath.getPathTo(path).toStr());
  foreach (const QString& filename, dir.entryList(QDir::Files | QDir::Hidden)) {
    if (!isRemoved(dirpath % filename)) {
      filenames.insert(filename);
    }
  }

  // add new files
  foreach (const QString& filepath, mModifiedFiles.keys()) {
    if (filepath.startsWith(dirpath)) {
      QStringList relpath = filepath.mid(dirpath.length()).split('/');
      if (relpath.count() == 1) {
        filenames.insert(relpath.first());
      }
    }
  }

  return filenames.values();
}

bool TransactionalFileSystem::fileExists(const QString& path) const noexcept {
  QString cleanedPath = cleanPath(path);
  if (mModifiedFiles.contains(cleanedPath)) {
    return true;
  } else if (isRemoved(cleanedPath)) {
    return false;
  } else {
    return mFilePath.getPathTo(cleanedPath).isExistingFile();
  }
}

QByteArray TransactionalFileSystem::read(const QString& path) const {
  QString cleanedPath = cleanPath(path);
  if (mModifiedFiles.contains(cleanedPath)) {
    return mModifiedFiles.value(cleanedPath);
  } else if (!isRemoved(cleanedPath)) {
    return FileUtils::readFile(mFilePath.getPathTo(cleanedPath));  // can throw
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("File '%1' does not exist.")
                           .arg(mFilePath.getPathTo(cleanedPath).toNative()));
  }
}

void TransactionalFileSystem::write(const QString& path,
                                    const QByteArray& content) {
  QString cleanedPath = cleanPath(path);
  mModifiedFiles[cleanedPath] = content;
  mRemovedFiles.remove(cleanedPath);
}

void TransactionalFileSystem::removeFile(const QString& path) {
  QString cleanedPath = cleanPath(path);
  mModifiedFiles.remove(cleanedPath);
  mRemovedFiles.insert(cleanedPath);
}

void TransactionalFileSystem::removeDirRecursively(const QString& path) {
  QString dirpath = cleanPath(path);
  if (!dirpath.isEmpty()) dirpath.append("/");
  foreach (const QString& fp, mModifiedFiles.keys()) {
    if (dirpath.isEmpty() || fp.startsWith(dirpath)) {
      mModifiedFiles.remove(fp);
    }
  }
  foreach (const QString& fp, mRemovedFiles) {
    if (dirpath.isEmpty() || fp.startsWith(dirpath)) {
      mRemovedFiles.remove(fp);
    }
  }
  mRemovedDirs.insert(dirpath);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void TransactionalFileSystem::loadFromZip(QByteArray content) {
  QBuffer buffer(&content);
  QuaZip zip(&buffer);
  if (!zip.open(QuaZip::mdUnzip)) {
    throw RuntimeError(__FILE__, __LINE__, tr("Failed to open ZIP file '%1'."));
  }
  QuaZipFile file(&zip);
  for (bool f = zip.goToFirstFile(); f; f = zip.goToNextFile()) {
    file.open(QIODevice::ReadOnly);
    write(file.getActualFileName(), file.readAll());
    file.close();
  }
  zip.close();
}

void TransactionalFileSystem::loadFromZip(const FilePath& fp) {
  QuaZip zip(fp.toStr());
  if (!zip.open(QuaZip::mdUnzip)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Failed to open the ZIP file '%1'.").arg(fp.toNative()));
  }
  QuaZipFile file(&zip);
  for (bool f = zip.goToFirstFile(); f; f = zip.goToNextFile()) {
    file.open(QIODevice::ReadOnly);
    write(file.getActualFileName(), file.readAll());
    file.close();
  }
  zip.close();
}

QByteArray TransactionalFileSystem::exportToZip() const {
  FilePath fp = FilePath::getRandomTempPath();
  QBuffer buffer;
  QuaZip zip(&buffer);
  if (!zip.open(QuaZip::mdCreate)) {
    throw RuntimeError(__FILE__, __LINE__, tr("Failed to create ZIP file."));
  }
  try {
    QuaZipFile file(&zip);
    exportDirToZip(file, fp, "");
    zip.close();
  } catch (const Exception& e) {
    // Remove ZIP file because it is not complete
    QFile(fp.toStr()).remove();
    zip.close();
    throw;
  }
  return buffer.buffer();
}

void TransactionalFileSystem::exportToZip(const FilePath& fp) const {
  QuaZip zip(fp.toStr());
  if (!zip.open(QuaZip::mdCreate)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Failed to create the ZIP file '%1'.").arg(fp.toNative()));
  }
  try {
    QuaZipFile file(&zip);
    exportDirToZip(file, fp, "");
    zip.close();
  } catch (const Exception& e) {
    // Remove ZIP file because it is not complete
    QFile(fp.toStr()).remove();
    zip.close();
    throw;
  }
}

void TransactionalFileSystem::discardChanges() noexcept {
  mModifiedFiles.clear();
  mRemovedFiles.clear();
  mRemovedDirs.clear();
}

QStringList TransactionalFileSystem::checkForModifications() const {
  QStringList modifications;

  // removed directories
  foreach (const QString& dir, mRemovedDirs) {
    FilePath fp = mFilePath.getPathTo(dir);
    if (fp.isExistingDir()) {
      modifications.append(dir);
    }
  }

  // removed files
  foreach (const QString& filepath, mRemovedFiles) {
    FilePath fp = mFilePath.getPathTo(filepath);
    if (fp.isExistingFile()) {
      modifications.append(filepath);
    }
  }

  // new or modified files
  foreach (const QString& filepath, mModifiedFiles.keys()) {
    FilePath fp = mFilePath.getPathTo(filepath);
    QByteArray content = mModifiedFiles.value(filepath);
    if ((!fp.isExistingFile()) ||
        (FileUtils::readFile(fp) != content)) {  // can throw
      modifications.append(filepath);
    }
  }

  return modifications;
}

void TransactionalFileSystem::autosave() {
  saveDiff("autosave");  // can throw
}

void TransactionalFileSystem::save() {
  // save to backup directory
  saveDiff("backup");  // can throw

  // modifications are now saved to the backup directory, so there is no risk
  // of loosing a restored autosave backup, thus we can reset its flag
  mRestoredFromAutosave = false;

  // remove autosave directory because it is now older than the backup content
  // (the user should not be able to restore the outdated autosave backup)
  removeDiff("autosave");  // can throw

  // remove directories
  foreach (const QString& dir, mRemovedDirs) {
    FilePath fp = mFilePath.getPathTo(dir);
    if (fp.isExistingDir()) {
      FileUtils::removeDirRecursively(fp);  // can throw
    }
  }

  // remove files
  foreach (const QString& filepath, mRemovedFiles) {
    FilePath fp = mFilePath.getPathTo(filepath);
    if (fp.isExistingFile()) {
      FileUtils::removeFile(fp);  // can throw
    }
  }

  // save new or modified files
  foreach (const QString& filepath, mModifiedFiles.keys()) {
    FileUtils::writeFile(mFilePath.getPathTo(filepath),
                         mModifiedFiles.value(filepath));  // can throw
  }

  // remove backup
  removeDiff("backup");  // can throw

  // clear state
  discardChanges();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QString TransactionalFileSystem::cleanPath(QString path) noexcept {
  return path.trimmed()
      .replace('\\', '/')
      .split('/', QString::SkipEmptyParts)
      .join('/')
      .trimmed();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool TransactionalFileSystem::isRemoved(const QString& path) const noexcept {
  if (mRemovedFiles.contains(path)) {
    return true;
  }

  foreach (const QString dir, mRemovedDirs) {
    if (path.startsWith(dir)) {
      return true;
    }
  }

  return false;
}

void TransactionalFileSystem::exportDirToZip(QuaZipFile& file,
                                             const FilePath& zipFp,
                                             const QString& dir) const {
  QString path = dir.isEmpty() ? dir : dir % "/";

  // export directories
  foreach (const QString& dirname, getDirs(dir)) {
    // skip dotdirs, e.g. ".git", ".svn", ".autosave", ".backup"
    if (dirname.startsWith('.')) continue;
    exportDirToZip(file, zipFp, path % dirname);
  }

  // export files
  foreach (const QString& filename, getFiles(dir)) {
    QString filepath = path % filename;
    if (filepath == zipFp.toRelative(mFilePath)) {
      // In case the exported ZIP file is located inside this file system,
      // we have to skip it. Otherwise we would get a ZIP inside the ZIP file.
      continue;
    }
    // skip lock file
    if (filename == ".lock") continue;
    // read file content and add it to the ZIP archive
    const QByteArray& content = read(filepath);  // can throw
    QuaZipNewInfo newFileInfo(filepath);
    newFileInfo.setPermissions(QFileDevice::ReadOwner | QFileDevice::ReadGroup |
                               QFileDevice::ReadOther |
                               QFileDevice::WriteOwner);
    if (!file.open(QIODevice::WriteOnly, newFileInfo)) {
      throw RuntimeError(__FILE__, __LINE__);
    }
    qint64 bytesWritten = file.write(content);
    file.close();
    if (bytesWritten != content.length()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Failed to write file '%1' to '%2'.")
                             .arg(filepath, zipFp.toNative()));
    }
  }
}

void TransactionalFileSystem::saveDiff(const QString& type) const {
  QDateTime dt = QDateTime::currentDateTime();
  FilePath dir = mFilePath.getPathTo("." % type);
  FilePath filesDir = dir.getPathTo(dt.toString("yyyy-MM-dd_hh-mm-ss-zzz"));

  if (!mIsWritable) {
    throw RuntimeError(__FILE__, __LINE__, tr("File system is read-only."));
  }

  SExpression root = SExpression::createList("librepcb_" % type);
  root.appendChild("created", dt, true);
  root.appendChild("modified_files_directory", filesDir.getFilename(), true);
  foreach (const QString& filepath, Toolbox::sorted(mModifiedFiles.keys())) {
    root.appendChild("modified_file", filepath, true);
    FileUtils::writeFile(filesDir.getPathTo(filepath),
                         mModifiedFiles.value(filepath));  // can throw
  }
  foreach (const QString& filepath, Toolbox::sorted(mRemovedFiles.values())) {
    root.appendChild("removed_file", filepath, true);
  }
  foreach (const QString& filepath, Toolbox::sorted(mRemovedDirs.values())) {
    root.appendChild("removed_directory", filepath, true);
  }

  // Writing the main file must be the last operation to "mark" this diff as
  // complete!
  FileUtils::writeFile(dir.getPathTo(type % ".lp"),
                       root.toByteArray());  // can throw
}

void TransactionalFileSystem::loadDiff(const FilePath& fp) {
  discardChanges();  // get a clean state first

  SExpression root =
      SExpression::parse(FileUtils::readFile(fp), fp);  // can throw
  QString modifiedFilesDirName =
      root.getValueByPath<QString>("modified_files_directory");
  FilePath modifiedFilesDir = fp.getParentDir().getPathTo(modifiedFilesDirName);
  foreach (const SExpression& node, root.getChildren("modified_file")) {
    QString relPath = node.getValueOfFirstChild<QString>();
    FilePath absPath = modifiedFilesDir.getPathTo(relPath);
    mModifiedFiles.insert(relPath, FileUtils::readFile(absPath));  // can throw
  }
  foreach (const SExpression& node, root.getChildren("removed_file")) {
    QString relPath = node.getValueOfFirstChild<QString>();
    mRemovedFiles.insert(relPath);
  }
  foreach (const SExpression& node, root.getChildren("removed_directory")) {
    QString relPath = node.getValueOfFirstChild<QString>();
    mRemovedDirs.insert(relPath);
  }
}

void TransactionalFileSystem::removeDiff(const QString& type) {
  FilePath dir = mFilePath.getPathTo("." % type);
  FilePath file = dir.getPathTo(type % ".lp");

  // remove the index file first to mark the diff directory as incomplete
  if (file.isExistingFile()) {
    FileUtils::removeFile(file);  // can throw
  }

  // then remove the whole directory
  FileUtils::removeDirRecursively(dir);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
