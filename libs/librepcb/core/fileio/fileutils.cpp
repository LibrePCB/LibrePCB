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
#include "fileutils.h"

#include "../exceptions.h"
#include "filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QByteArray FileUtils::readFile(const FilePath& filepath) {
  if (!filepath.isExistingFile()) {
    throw LogicError(
        __FILE__, __LINE__,
        tr("The file \"%1\" does not exist.").arg(filepath.toNative()));
  }
  QFile file(filepath.toStr());
  if (!file.open(QIODevice::ReadOnly)) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Cannot "
                          "open file \"%1\": %2")
                           .arg(filepath.toNative(), file.errorString()));
  }
  return file.readAll();
}

void FileUtils::writeFile(const FilePath& filepath, const QByteArray& content) {
  makePath(filepath.getParentDir());  // can throw
  QSaveFile file(filepath.toStr());
  if (!file.open(QIODevice::WriteOnly)) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not open or create file \"%1\": %2")
                           .arg(filepath.toNative(), file.errorString()));
  }
  qint64 written = file.write(content);
  if (written != content.size()) {
    qDebug() << "Only" << written << "of" << content.size() << "bytes written.";
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not write to file \"%1\": %2")
                           .arg(filepath.toNative(), file.errorString()));
  }
  if (!file.commit()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not write to "
                          "file \"%1\": %2")
                           .arg(filepath.toNative(), file.errorString()));
  }
}

void FileUtils::copyFile(const FilePath& source, const FilePath& dest) {
  if (!source.isExistingFile()) {
    throw LogicError(
        __FILE__, __LINE__,
        tr("The file \"%1\" does not exist.").arg(source.toNative()));
  }
  if (dest.isExistingFile() || dest.isExistingDir()) {
    throw LogicError(__FILE__, __LINE__,
                     tr("The file or directory \"%1\" exists already.")
                         .arg(dest.toNative()));
  }
  if (!QFile::copy(source.toStr(), dest.toStr())) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not copy file \"%1\" to \"%2\".")
                           .arg(source.toNative(), dest.toNative()));
  }
}

void FileUtils::copyDirRecursively(const FilePath& source,
                                   const FilePath& dest) {
  if (!source.isExistingDir()) {
    throw LogicError(
        __FILE__, __LINE__,
        tr("The directory \"%1\" does not exist.").arg(source.toNative()));
  }
  if (dest.isExistingFile() || dest.isExistingDir()) {
    throw LogicError(__FILE__, __LINE__,
                     tr("The file or directory \"%1\" exists already.")
                         .arg(dest.toNative()));
  }
  makePath(dest);  // can throw
  QDir sourceDir(source.toStr());
  foreach (const QString& file,
           sourceDir.entryList(QDir::Files | QDir::Hidden)) {
    copyFile(source.getPathTo(file), dest.getPathTo(file));
  }
  foreach (const QString& dir,
           sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
    copyDirRecursively(source.getPathTo(dir), dest.getPathTo(dir));
  }
}

void FileUtils::move(const FilePath& source, const FilePath& dest) {
  if ((!source.isExistingFile()) && (!source.isExistingDir())) {
    throw LogicError(__FILE__, __LINE__,
                     tr("The file or directory \"%1\" does not exist.")
                         .arg(source.toNative()));
  }
  if (dest.isExistingFile() || dest.isExistingDir()) {
    throw LogicError(__FILE__, __LINE__,
                     tr("The file or directory \"%1\" exists already.")
                         .arg(dest.toNative()));
  }
  // Note: QDir::rename() fails if the parent directory does not yet exist
  makePath(dest.getParentDir());
  if (!QDir().rename(source.toStr(), dest.toStr())) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not move \"%1\" to \"%2\".")
                           .arg(source.toNative(), dest.toNative()));
  }
}

void FileUtils::removeFile(const FilePath& file) {
  if (!QFile::remove(file.toStr())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not remove file \"%1\".").arg(file.toNative()));
  }
}

void FileUtils::removeDirRecursively(const FilePath& dir) {
  if (!QDir(dir.toStr()).removeRecursively()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not remove directory \"%1\".").arg(dir.toNative()));
  }
}

void FileUtils::makePath(const FilePath& path) {
  if (!QDir().mkpath(path.toStr())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not create directory or path \"%1\".").arg(path.toNative()));
  }
}

QList<FilePath> FileUtils::getFilesInDirectory(const FilePath& dir,
                                               const QStringList& filters,
                                               bool recursive) {
  if (!dir.isExistingDir()) {
    throw LogicError(
        __FILE__, __LINE__,
        tr("The directory \"%1\" does not exist.").arg(dir.toNative()));
  }

  QList<FilePath> files;
  QDir qDir(dir.toStr());
  qDir.setFilter(QDir::Files | QDir::Hidden | QDir::Dirs |
                 QDir::NoDotAndDotDot);
  if (!filters.isEmpty()) qDir.setNameFilters(filters);
  foreach (const QFileInfo& info, qDir.entryInfoList()) {
    FilePath fp(info.absoluteFilePath());
    if (info.isFile()) {
      files.append(fp);
    } else if (info.isDir() && recursive) {
      files += getFilesInDirectory(fp, filters, recursive);
    }
  }
  return files;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
