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

#ifndef LIBREPCB_EDITOR_RPCHELPERS_H
#define LIBREPCB_EDITOR_RPCHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpctypes.h"

#include <librepcb/core/types/uuid.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/rust-core/rpcext.h>
#include <type_traits>

#include <QtCore>

#include <optional>
#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  RPC Helpers
 ******************************************************************************/

/// Convert a string list into the protocol's list-of-strings
/// representation (QStringList is not part of the protocol's value
/// domain, see the RPC API specification).
inline QVariant toVariant(const QStringList& list) noexcept {
  return QVariant(list).value<QVariantList>();
}

template <typename T, typename F>
inline QVariantList toVariant(const T& list, F convertItem) {
  QVariantList result;
  for (const auto& obj : list) {
    if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(obj)>>) {
      result.append(convertItem(*obj));
    } else {
      result.append(convertItem(obj));
    }
  }
  return result;
}

/// Convert a Uuid into the protocol's "uuid" extension type (see the API
/// specification): An ext value with the 16 raw UUID bytes in RFC 4122
/// field order.
inline QVariant toVariant(const Uuid& obj) noexcept {
  return QVariant::fromValue(
      rs::MsgPackExt(kMsgPackExtTypeUuid, QUuid(obj.toStr()).toRfc4122()));
}

inline QVariant toVariant(const FilePath& obj) noexcept {
  return obj.isValid() ? obj.toStr() : QVariant();
}

template <typename T, typename F>
inline QVariantList modelToVariant(const slint::Model<T>& model,
                                   F convertItem) {
  QVariantList result;
  for (std::size_t i = 0; i < model.row_count(); ++i) {
    if (auto obj = model.row_data(i)) {
      result.append(convertItem(*obj));
    }
  }
  return result;
}

/*******************************************************************************
 *  Class RpcArgTraits
 ******************************************************************************/

/**
 * @brief How to read an argument of type T from a request
 *
 * Specialized for every supported argument type (see the API
 * specification). Arguments of any other type fail to compile.
 */
template <typename T>
struct RpcArgTraits;

template <>
struct RpcArgTraits<QString> {
  static constexpr const char* NAME = "string";
  static std::optional<QString> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QString) return v.toString();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<bool> {
  static constexpr const char* NAME = "bool";
  static std::optional<bool> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::Bool) return v.toBool();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<qint64> {
  static constexpr const char* NAME = "int";
  static std::optional<qint64> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::LongLong) return v.toLongLong();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<double> {
  static constexpr const char* NAME = "float";
  static std::optional<double> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::Double) return v.toDouble();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<QByteArray> {
  static constexpr const char* NAME = "binary";
  static std::optional<QByteArray> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QByteArray) return v.toByteArray();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<QVariantMap> {
  static constexpr const char* NAME = "map";
  static std::optional<QVariantMap> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QVariantMap) return v.toMap();
    return std::nullopt;
  }
};

/// The protocol's "uuid" extension type — strictly the ext value, i.e.
/// a 36-character string is *not* accepted (see the API specification).
template <>
struct RpcArgTraits<Uuid> {
  static constexpr const char* NAME = "uuid";
  static std::optional<Uuid> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::fromType<rs::MsgPackExt>().id()) {
      const auto ext = v.value<rs::MsgPackExt>();
      if ((ext.getType() == kMsgPackExtTypeUuid) &&
          (ext.getData().size() == 16)) {
        const QUuid uuid = QUuid::fromRfc4122(ext.getData());
        return Uuid::tryFromString(uuid.toString(QUuid::WithoutBraces));
      }
    }
    return std::nullopt;
  }
};

/// Any enum type — delegates to the generated RpcEnumTraits.
template <typename T>
  requires std::is_enum_v<T>
struct RpcArgTraits<T> {
  static constexpr const char* NAME = RpcEnumTraits<T>::NAME;
  static std::optional<T> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QString) {
      const auto& map = RpcEnumTraits<T>::mapFromString();
      const auto it = map.constFind(v.toString());
      if (it != map.constEnd()) return *it;
    }
    return std::nullopt;
  }
};

/// A list of any supported type.
template <typename T>
struct RpcArgTraits<QList<T>> {
  static constexpr const char* NAME = "array";
  static std::optional<QList<T>> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() != QMetaType::QVariantList) return std::nullopt;
    QList<T> result;
    for (const auto& item : v.toList()) {
      if (auto converted = RpcArgTraits<T>::tryFromVariant(item)) {
        result.append(*converted);
      } else {
        return std::nullopt;
      }
    }
    return result;
  }
};

/// A set of any supported type.
template <typename T>
struct RpcArgTraits<QSet<T>> {
  static constexpr const char* NAME = "array";
  static std::optional<QSet<T>> tryFromVariant(const QVariant& v) noexcept {
    if (auto list = RpcArgTraits<QList<T>>::tryFromVariant(v)) {
      const QSet<T> set(list->begin(), list->end());
      if (set.count() == list->count()) {
        return set;
      }
    }
    return std::nullopt;
  }
};

/*******************************************************************************
 *  Typed Argument Reading
 ******************************************************************************/

/// Read an optional argument, returning std::nullopt if it is not
/// provided at all. If it is provided but has an invalid value, an
/// InvalidArgs error is thrown.
template <typename T>
std::optional<T> getOptionalArg(const RpcRequest& request,
                                const QString& argName) {
  const auto& args = request.getArgs();
  const auto it = args.constFind(argName);
  if (it == args.constEnd()) {
    return std::nullopt;
  }
  if (auto converted = RpcArgTraits<T>::tryFromVariant(*it)) {
    return *converted;
  }
  if constexpr (std::is_enum_v<T>) {
    throw RpcInvalidArgs(
        QString("Unknown value '%1' for argument '%2' of enum %3. "
                "Supported values are [%4].")
            .arg(it->toString(), argName, RpcEnumTraits<T>::NAME,
                 RpcEnumTraits<T>::mapFromString().keys().join(", ")));
  } else {
    throw RpcInvalidArgs(
        QString("Invalid value '%1' for argument '%2' (expected %3).")
            .arg(it->toString(), argName, RpcArgTraits<T>::NAME));
  }
}

/// Read a mandatory argument. If it is not provided, an InvalidArgs error
/// is thrown. If it is provided but has an invalid value (e.g. nil, or a
/// wrong type), an InvalidArgs error is thrown as well.
template <typename T>
T getArg(const RpcRequest& request, const QString& argName) {
  if (auto value = getOptionalArg<T>(request, argName)) {
    return *value;
  }
  throw RpcInvalidArgs(QString("Missing value for argument '%1' (expected %2).")
                           .arg(argName, RpcArgTraits<T>::NAME));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb

#endif
