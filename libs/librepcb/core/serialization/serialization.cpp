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
#include "serialization.h"

#include <QString>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Serialization
 ******************************************************************************/

// Note: Only C++/Qt types are implemented here. Implementations for LibrePCB
// types are located right in the corresponding class files.

template <>
QString serializeToString(const bool& obj) noexcept {
  return obj ? QStringLiteral("true") : QStringLiteral("false");
}

template <>
std::optional<bool> deserialize(QStringView str) noexcept {
  if (str == QStringLiteral("true")) {
    return true;
  } else if (str == QStringLiteral("false")) {
    return false;
  } else {
    return std::nullopt;
  }
}

template <>
QString serializeToString(const QString& obj) noexcept {
  return obj;
}

template <>
std::optional<QString> deserialize(QStringView str) noexcept {
  return str.toString();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
