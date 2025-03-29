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
#include "ziparchive.h"

#include "../exceptions.h"
#include "filepath.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ZipArchive
 ******************************************************************************/

RustHandle<rs::ZipArchive> construct(const QByteArray& data) {
  QString err;
  if (auto obj = rs::ffi_ziparchive_new_from_mem(&data, &err)) {
    return RustHandle<rs::ZipArchive>(*obj, &rs::ffi_ziparchive_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__, "Failed to open Zip file: " % err);
  }
}

static RustHandle<rs::ZipArchive> construct(const FilePath& fp) {
  const QString path = fp.toStr();
  QString err;
  if (auto obj = rs::ffi_ziparchive_new_from_file(&path, &err)) {
    return RustHandle<rs::ZipArchive>(*obj, &rs::ffi_ziparchive_delete);
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Failed to open Zip file '%1': %2").arg(fp.toNative(), err));
  }
}

ZipArchive::ZipArchive(const QByteArray& data) : mHandle(construct(data)) {
}

ZipArchive::ZipArchive(const FilePath& fp) : mHandle(construct(fp)) {
}

std::size_t ZipArchive::getEntriesCount() const noexcept {
  return rs::ffi_ziparchive_len(*mHandle);
}

QString ZipArchive::getFileName(std::size_t index) {
  QString name;
  QString err;
  if (!rs::ffi_ziparchive_name_for_index(*mHandle, index, &name, &err)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to get file name from Zip: " % err);
  }
  return name;
}

QByteArray ZipArchive::readFile(std::size_t index) {
  QByteArray buf;
  QString err;
  if (!rs::ffi_ziparchive_read_by_index(*mHandle, index, &buf, &err)) {
    throw RuntimeError(__FILE__, __LINE__, "Failed to read from Zip: " % err);
  }
  return buf;
}

std::optional<QByteArray> ZipArchive::tryReadFile(const QString& fileName) {
  for (std::size_t i = 0; i < getEntriesCount(); ++i) {
    if (getFileName(i) == fileName) {
      return readFile(i);
    }
  }
  return std::nullopt;
}

void ZipArchive::extractTo(const FilePath& dir) {
  const QString path = dir.toStr();
  if (!rs::ffi_ziparchive_extract(*mHandle, &path)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Failed to extract Zip archive to '%1'.").arg(dir.toNative()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
