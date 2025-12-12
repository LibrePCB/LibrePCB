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

#ifndef LIBREPCB_CORE_BUSNAME_H
#define LIBREPCB_CORE_BUSNAME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BusName
 ******************************************************************************/

struct BusNameVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(__FILE__, __LINE__,
                              QString(QCoreApplication::translate(
                                          "BusName", "Invalid bus name: '%1'"))
                                  .arg(val)),
           std::forward<Value>(val));
  }
};

struct BusNameConstraint {
  static const constexpr int MAX_LENGTH = 32;

  bool operator()(const QString& value) const noexcept {
    // Same as CircuitIdentifier, but adding []
    return QRegularExpression("\\A[-a-zA-Z0-9._+/!?&@#$()\\[\\]]{1,32}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * BusName is a wrapper around QString which guarantees to contain a
 * valid bus name
 *
 * The rules for a valid bus name are exactly the same as for
 * ::librepcb::CircuitIdentifier, with the only difference that the
 * characters '[' and ']' are also allowed (to denote vectors).
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid bus name.
 */
using BusName =
    type_safe::constrained_type<QString, BusNameConstraint, BusNameVerifier>;

inline bool operator==(const BusName& lhs, const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs, const BusName& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const BusName& lhs, const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs, const BusName& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const BusName& lhs, const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs, const BusName& rhs) noexcept {
  return lhs % (*rhs);
}
inline QString operator%(const BusName& lhs, const BusName& rhs) noexcept {
  return (*lhs) % (*rhs);
}

template <>
inline std::unique_ptr<SExpression> serialize(const BusName& obj) {
  return SExpression::createString(*obj);
}

template <>
inline BusName deserialize(const SExpression& node) {
  return BusName(node.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const BusName& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const BusName& obj) {
  stream << QString("BusName('%1')").arg(*obj);
  return stream;
}

inline std::size_t qHash(const BusName& key, std::size_t seed = 0) noexcept {
  return ::qHash(*key, seed);
}

inline static QString cleanBusName(const QString& userInput) noexcept {
  return Toolbox::cleanUserInputString(
      userInput, QRegularExpression("[^-a-zA-Z0-9._+/!?&@#$()\\[\\]]"), true,
      false, false, "_", BusNameConstraint::MAX_LENGTH);
}

inline static std::optional<BusName> parseBusName(
    const QString& name) noexcept {
  return BusNameConstraint()(name) ? BusName(name) : std::optional<BusName>();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
