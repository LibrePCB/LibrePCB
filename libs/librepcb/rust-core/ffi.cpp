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
#include "ffi.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace rs {

/*******************************************************************************
 *  Functions
 ******************************************************************************/

extern "C" std::size_t ffi_qbytearray_len(const QByteArray* obj) {
  return static_cast<std::size_t>(obj->size());
}

extern "C" const uint8_t* ffi_qbytearray_data(const QByteArray* obj) {
  return reinterpret_cast<const uint8_t*>(obj->data());
}

extern "C" uint8_t* ffi_qbytearray_data_mut(QByteArray* obj) {
  return reinterpret_cast<uint8_t*>(obj->data());
}

extern "C" void ffi_qbytearray_resize(QByteArray* obj, std::size_t len,
                                      uint8_t value) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
  obj->resize(len, value);
#else
  if (len > static_cast<std::size_t>(obj->size())) {
    obj->append(len - obj->size(), value);
  } else {
    obj->resize(len);
  }
#endif
}

extern "C" QString* ffi_qstring_new(const char* s) {
  return new QString(s);
}

extern "C" uintptr_t ffi_qstring_len(const QString* obj) {
  return obj->size();
}

extern "C" const uint16_t* ffi_qstring_utf16(const QString* obj) {
  return obj->utf16();
}

extern "C" void ffi_qstring_set(QString* obj, const char* s, std::size_t len) {
  *obj = QString::fromUtf8(s, len);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rs
}  // namespace librepcb
