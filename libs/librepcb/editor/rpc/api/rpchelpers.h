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
#include "rpcerrors.h"
#include "rpctypes.h"

#include <librepcb/core/types/uuid.h>
#include <librepcb/rust-core/rpcext.h>

#include <QtCore>

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

/// Convert a Uuid into the protocol's "uuid" extension type (see the API
/// specification): An ext value with the 16 raw UUID bytes in RFC 4122
/// field order.
inline QVariant toVariant(const Uuid& uuid) noexcept {
  return QVariant::fromValue(
      rs::MsgPackExt(kMsgPackExtTypeUuid, QUuid(uuid.toStr()).toRfc4122()));
}

inline Uuid toUuid(const QVariant& value) {
  const auto ext = value.value<rs::MsgPackExt>();
  if (ext.getType() != kMsgPackExtTypeUuid) {
    throw RpcInvalidArgs("Not a UUID extension type.");
  }
  const QUuid uuid = QUuid::fromRfc4122(ext.getData());
  if (uuid.isNull()) {
    throw RpcInvalidArgs("Invalid UUID.");
  }
  return Uuid::fromString(uuid.toString(QUuid::WithoutBraces));
}

template <typename T>
QList<T> toList(const QVariant& value) {
  // TODO
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb

#endif
