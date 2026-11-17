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

#ifndef LIBREPCB_CORE_SERIALIZATION_H
#define LIBREPCB_CORE_SERIALIZATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QString>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Serialization for Basic Types
 ******************************************************************************/

/**
 * @brief Serialize a value object into a plain string
 *
 * Unlike ::librepcb::serialize() (which produces an ::librepcb::SExpression),
 * this is intended for lightweight persistence like in `QSettings`. Value
 * objects (like enums, ::librepcb::Length etc.) may or may not provide an
 * implementation for this template. A specialization needs to be added to
 * the types this serialization is needed for.
 *
 * @attention The returned strings must be stable across releases, otherwise
 *            we would get errors due to incompatible files!
 *
 * @tparam T    Type of object to serialize.
 * @param obj   Object to serialize.
 * @return      Serialized string.
 */
template <typename T>
QString serializeToString(const T& obj) noexcept;

/**
 * @brief Deserialize a value object from a plain string
 *
 * Counterpart to ::librepcb::serializeToString().
 *
 * @tparam T    Type of object to deserialize.
 * @param str   String to deserialize, as returned by
 *              ::librepcb::serializeToString().
 * @return      Deserialized object, or `std::nullopt` if the string is not
 *              a valid representation.
 */
template <typename T>
std::optional<T> deserialize(QStringView str) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
