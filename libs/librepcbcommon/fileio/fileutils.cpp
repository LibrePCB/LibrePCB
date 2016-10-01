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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "fileutils.h"
#include "filepath.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QByteArray FileUtils::readFile(const FilePath& filepath) throw (Exception)
{
    if (!filepath.isExistingFile()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file \"%1\" does not exist."))
            .arg(filepath.toNative()));
    }
    QFile file(filepath.toStr());
    if (!file.open(QIODevice::ReadOnly)) {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr("Cannot "
            "open file \"%1\": %2")).arg(filepath.toNative(), file.errorString()));
    }
    return file.readAll();
}

void FileUtils::writeFile(const FilePath& filepath, const QByteArray& content) throw (Exception)
{
    makePath(filepath.getParentDir()); // can throw
    QSaveFile file(filepath.toStr());
    if (!file.open(QIODevice::WriteOnly)) {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
            .arg(filepath.toStr(), file.errorString()).arg(file.error()),
            QString(tr("Could not open or create file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }
    qint64 written = file.write(content);
    if (written != content.size()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1: %2 (only %3 of %4 bytes written)")
            .arg(filepath.toStr(), file.errorString()).arg(written).arg(content.size()),
            QString(tr("Could not write to file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }
    if (!file.commit()) {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr("Could not write to "
            "file \"%1\": %2")).arg(filepath.toNative(), file.errorString()));
    }
}

void FileUtils::copyFile(const FilePath& source, const FilePath& dest) throw (Exception)
{
    if (!source.isExistingFile()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file \"%1\" does not exist."))
            .arg(source.toNative()));
    }
    if (dest.isExistingFile() || dest.isExistingDir()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file or directory \"%1\" exists already."))
            .arg(dest.toNative()));
    }
    if (!QFile::copy(source.toStr(), dest.toStr())) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Could not copy file \"%1\" to \"%2\"."))
            .arg(source.toNative(), dest.toNative()));
    }
}

void FileUtils::copyDirRecursively(const FilePath& source, const FilePath& dest) throw (Exception)
{
    if (!source.isExistingDir()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The directory \"%1\" does not exist."))
            .arg(source.toNative()));
    }
    if (dest.isExistingFile() || dest.isExistingDir()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file or directory \"%1\" exists already."))
            .arg(dest.toNative()));
    }
    makePath(dest); // can throw
    QDir sourceDir(source.toStr());
    foreach (const QString& file, sourceDir.entryList(QDir::Files | QDir::Hidden)) {
        copyFile(source.getPathTo(file), dest.getPathTo(file));
    }
    foreach (const QString& dir, sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        copyDirRecursively(source.getPathTo(dir), dest.getPathTo(dir));
    }
}

void FileUtils::move(const FilePath& source, const FilePath& dest) throw (Exception)
{
    if ((!source.isExistingFile()) && (!source.isExistingDir())) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file or directory \"%1\" does not exist."))
            .arg(source.toNative()));
    }
    if (dest.isExistingFile() || dest.isExistingDir()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            QString(tr("The file or directory \"%1\" exists already."))
            .arg(dest.toNative()));
    }
    if (!QDir().rename(source.toStr(), dest.toStr())) {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr(
            "Could not move \"%1\" to \"%2\".")).arg(source.toNative(), dest.toNative()));
    }
}

void FileUtils::removeFile(const FilePath& file) throw (Exception)
{
    if (!QFile::remove(file.toStr())) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Could not remove file \"%1\".")).arg(file.toNative()));
    }
}

void FileUtils::removeDirRecursively(const FilePath& dir) throw (Exception)
{
    if (!QDir(dir.toStr()).removeRecursively()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Could not remove directory \"%1\"."))
            .arg(dir.toNative()));
    }
}

void FileUtils::makePath(const FilePath& path) throw (Exception)
{
    if (!QDir().mkpath(path.toStr())) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Could not create directory or path \"%1\"."))
            .arg(path.toNative()));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
