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

#include <QtCore>

#include <cstring>
#include <iterator>

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

extern "C" std::size_t ffi_qstring_len(const QString* obj) {
  return static_cast<std::size_t>(obj->size());
}

extern "C" const uint16_t* ffi_qstring_utf16(const QString* obj) {
  return obj->utf16();
}

extern "C" void ffi_qstring_set(QString* obj, const char* s, std::size_t len) {
  *obj = QString::fromUtf8(s, len);
}

extern "C" QString* ffi_qstring_new() {
  return new QString();
}

extern "C" void ffi_qstring_delete(QString* obj) {
  delete obj;
}

// QVariant interop for the RPC server. The RPC protocol only uses a
// restricted subset of the QVariant data model (nil, bool, i64, f64,
// string, bytearray, list, map with string keys), so these functions
// operate on exactly that subset. Note that all containers must be built
// up incrementally (insert/append) since Rust can't construct QVariant
// containers directly.

namespace {

/// Get the map contained in a QVariant (only valid if the type matches!).
QVariantMap& mapData(QVariant* obj) {
  return *static_cast<QVariantMap*>(obj->data());
}

/// Get the list contained in a QVariant (only valid if the type matches!).
QVariantList& listData(QVariant* obj) {
  return *static_cast<QVariantList*>(obj->data());
}

/// Get the map contained in a QVariant (only valid if the type matches!).
const QVariantMap& mapData(const QVariant* obj) {
  return *static_cast<const QVariantMap*>(obj->constData());
}

/// Get the list contained in a QVariant (only valid if the type matches!).
const QVariantList& listData(const QVariant* obj) {
  return *static_cast<const QVariantList*>(obj->constData());
}

}  // namespace

extern "C" QVariant* ffi_qvariant_new() {
  return new QVariant();  // An invalid QVariant represents nil.
}

extern "C" void ffi_qvariant_delete(QVariant* obj) {
  delete obj;
}

extern "C" void ffi_qvariant_set_nil(QVariant* obj) {
  *obj = QVariant();
}

extern "C" void ffi_qvariant_set_bool(QVariant* obj, bool value) {
  *obj = QVariant(value);
}

extern "C" void ffi_qvariant_set_i64(QVariant* obj, int64_t value) {
  *obj = QVariant(static_cast<qlonglong>(value));
}

extern "C" void ffi_qvariant_set_f64(QVariant* obj, double value) {
  *obj = QVariant(value);
}

extern "C" void ffi_qvariant_set_str(QVariant* obj, const char* s,
                                     std::size_t len) {
  *obj = QVariant(QString::fromUtf8(s, static_cast<qsizetype>(len)));
}

extern "C" void ffi_qvariant_set_bytearray(QVariant* obj, const uint8_t* data,
                                           std::size_t len) {
  *obj = QVariant(QByteArray(reinterpret_cast<const char*>(data),
                             static_cast<qsizetype>(len)));
}

extern "C" void ffi_qvariant_set_empty_map(QVariant* obj) {
  *obj = QVariant(QVariantMap());
}

extern "C" void ffi_qvariant_set_empty_list(QVariant* obj) {
  *obj = QVariant(QVariantList());
}

extern "C" bool ffi_qvariant_map_insert(QVariant* obj, const char* key,
                                        std::size_t key_len,
                                        const QVariant* value) {
  if (obj->userType() != QMetaType::QVariantMap) {
    return false;
  }
  mapData(obj).insert(QString::fromUtf8(key, static_cast<qsizetype>(key_len)),
                      *value);
  return true;
}

extern "C" bool ffi_qvariant_list_append(QVariant* obj, const QVariant* value) {
  if (obj->userType() != QMetaType::QVariantList) {
    return false;
  }
  listData(obj).append(*value);
  return true;
}

extern "C" int ffi_qvariant_type(const QVariant* obj) {
  // Return the Qt metatype ID (a `QMetaType::Type` value) directly — the
  // Rust side matches on the IDs of the protocol's value domain (see the
  // `qmetatype` constants in cpp_ffi.rs) and rejects everything else.
  return obj->userType();
}

extern "C" bool ffi_qvariant_get_bool(const QVariant* obj) {
  return obj->toBool();
}

extern "C" int64_t ffi_qvariant_get_i64(const QVariant* obj) {
  return obj->toLongLong();
}

extern "C" double ffi_qvariant_get_f64(const QVariant* obj) {
  return obj->toDouble();
}

extern "C" void ffi_qvariant_get_str(const QVariant* obj, QString* out) {
  *out = obj->toString();
}

extern "C" std::size_t ffi_qvariant_get_bytearray_len(const QVariant* obj) {
  if (obj->userType() != QMetaType::QByteArray) {
    return 0;
  }
  const auto& ba = *static_cast<const QByteArray*>(obj->constData());
  return static_cast<std::size_t>(ba.size());
}

extern "C" void ffi_qvariant_get_bytearray_data(const QVariant* obj,
                                                uint8_t* out) {
  const auto& ba = *static_cast<const QByteArray*>(obj->constData());
  std::memcpy(out, ba.constData(), static_cast<std::size_t>(ba.size()));
}

extern "C" std::size_t ffi_qvariant_list_len(const QVariant* obj) {
  if (obj->userType() != QMetaType::QVariantList) {
    return 0;
  }
  return static_cast<std::size_t>(listData(obj).size());
}

extern "C" void ffi_qvariant_list_at(const QVariant* obj, std::size_t index,
                                     QVariant* out) {
  Q_ASSERT(obj->userType() == QMetaType::QVariantList);
  *out = listData(obj).at(static_cast<qsizetype>(index));
}

extern "C" std::size_t ffi_qvariant_map_len(const QVariant* obj) {
  if (obj->userType() != QMetaType::QVariantMap) {
    return 0;
  }
  return static_cast<std::size_t>(mapData(obj).size());
}

extern "C" void ffi_qvariant_map_at(const QVariant* obj, std::size_t index,
                                    QString* key, QVariant* out) {
  Q_ASSERT(obj->userType() == QMetaType::QVariantMap);
  const auto& map = mapData(obj);
  Q_ASSERT(index < static_cast<std::size_t>(map.size()));
  auto it = map.constBegin();
  std::advance(it, static_cast<qsizetype>(index));
  *key = it.key();
  *out = it.value();
}

extern "C" std::size_t ffi_qstringlist_len(const QStringList* obj) {
  return static_cast<std::size_t>(obj->size());
}

extern "C" const QString* ffi_qstringlist_at(const QStringList* obj,
                                             std::size_t index) {
  Q_ASSERT(index < static_cast<std::size_t>(obj->size()));
  return &obj->at(index);
}

extern "C" void ffi_qstringlist_append(QStringList* obj, const char* s,
                                       std::size_t len) {
  obj->append(QString::fromUtf8(s, len));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rs
}  // namespace librepcb
