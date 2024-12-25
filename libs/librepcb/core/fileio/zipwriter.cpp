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
#include "zipwriter.h"

#include "../exceptions.h"
#include "filepath.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ZipWriter
 ******************************************************************************/

static RustHandle<rs::ZipWriter> construct(QByteArray& data) {
  if (auto obj = rs::ffi_zipwriter_new_to_mem(&data)) {
    return RustHandle<rs::ZipWriter>(*obj, &rs::ffi_zipwriter_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__, "Failed to create Zip file.");
  }
}

static RustHandle<rs::ZipWriter> construct(const FilePath& fp) {
  const QString path = fp.toStr();
  QString err;
  if (auto obj = rs::ffi_zipwriter_new_to_file(&path, &err)) {
    return RustHandle<rs::ZipWriter>(*obj, &rs::ffi_zipwriter_delete);
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Failed to create Zip file '%1': %2").arg(fp.toNative(), err));
  }
}

ZipWriter::ZipWriter() : mHandle(construct(mBuffer)), mFinished(false) {
}

ZipWriter::ZipWriter(const FilePath& fp)
  : mHandle(construct(fp)), mFinished(false) {
}

ZipWriter::~ZipWriter() noexcept {
  if (!mFinished) {
    qWarning() << "ZipWriter destroyed without calling finish()!";
  }
}

void ZipWriter::writeFile(const QString& path, const QByteArray& data,
                          uint32_t mode) {
  QString err;
  if (!rs::ffi_zipwriter_write_file(*mHandle, &path, &data, mode, &err)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to write file in Zip: " % err);
  }
}

void ZipWriter::finish() {
  QString err;
  if (!rs::ffi_zipwriter_finish(*mHandle, &err)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to finish writing Zip: " % err);
  }
  mFinished = true;
}

const QByteArray& ZipWriter::getData() const {
  if (mBuffer.isEmpty() || (!mFinished)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Invalid or incomplete in-memory Zip file.");
  }
  return mBuffer;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
