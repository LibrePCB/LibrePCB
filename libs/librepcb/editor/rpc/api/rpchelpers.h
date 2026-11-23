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
#include <QtCore>

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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb

#endif
