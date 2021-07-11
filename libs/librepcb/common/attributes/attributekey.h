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

#ifndef LIBREPCB_ATTRIBUTEKEY_H
#define LIBREPCB_ATTRIBUTEKEY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"
#include "../toolbox.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AttributeKey
 ******************************************************************************/

inline static QString cleanAttributeKey(const QString& userInput) noexcept {
  return Toolbox::cleanUserInputString(
      userInput, QRegularExpression("[^_0-9A-Z]"), true, false, true, "_", 40);
}

struct AttributeKeyVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(
               __FILE__, __LINE__,
               QString(QApplication::translate("AttributeKey",
                                               "Invalid attribute key: '%1'"))
                   .arg(val)),
           std::forward<Value>(val));
  }
};

struct AttributeKeyConstraint {
  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("\\A[_0-9A-Z]{1,40}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * AttributeKey is a wrapper around QString which guarantees to contain a valid
 * key for librepcb::Attribute.
 *
 * An attribute key is considered as valid if it:
 *   - contains minimum 1 and maximum 40 characters
 *   - only contains the characters [A-Z] (uppercase), [0-9] or [_] (underscore)
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid attribute key according these rules.
 */
using AttributeKey =
    type_safe::constrained_type<QString, AttributeKeyConstraint,
                                AttributeKeyVerifier>;

inline bool operator==(const AttributeKey& lhs, const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs, const AttributeKey& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const AttributeKey& lhs, const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs, const AttributeKey& rhs) noexcept {
  return lhs != (*rhs);
}

template <>
inline SExpression serialize(const AttributeKey& obj) {
  return SExpression::createString(*obj);
}

template <>
inline AttributeKey deserialize(const SExpression& sexpr,
                                const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return AttributeKey(sexpr.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const AttributeKey& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const AttributeKey& obj) {
  stream << QString("AttributeKey('%1'')").arg(*obj);
  return stream;
}

inline uint qHash(const AttributeKey& key, uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTEKEY_H
