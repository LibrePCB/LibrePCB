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

#ifndef LIBREPCB_LIBRARY_COMPONENTPREFIX_H
#define LIBREPCB_LIBRARY_COMPONENTPREFIX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/sexpression.h>
#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class ComponentPrefix
 ******************************************************************************/

struct ComponentPrefixVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(
               __FILE__, __LINE__,
               QString(QApplication::translate(
                           "ComponentPrefix", "Invalid component prefix: '%1'"))
                   .arg(val)),
           std::forward<Value>(val));
  }
};

struct ComponentPrefixConstraint {
  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("\\A[a-zA-Z_]{0,16}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * ComponentPrefix is a wrapper around QString which guarantees to contain a
 * valid prefix used for librepcb::library::Component (e.g. "R" for a resistor)
 *
 * A component prefix is considered as valid if it:
 *   - contains only the characters [a-zA-Z_]
 *   - is not longer than 16 characters
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid component prefix according these rules.
 */
using ComponentPrefix =
    type_safe::constrained_type<QString, ComponentPrefixConstraint,
                                ComponentPrefixVerifier>;

}  // namespace library

inline bool operator==(const library::ComponentPrefix& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs,
                       const library::ComponentPrefix& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const library::ComponentPrefix& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs,
                       const library::ComponentPrefix& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const library::ComponentPrefix& lhs,
                         const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs,
                         const library::ComponentPrefix& rhs) noexcept {
  return lhs % (*rhs);
}

template <>
inline SExpression serialize(const library::ComponentPrefix& obj) {
  return SExpression::createString(*obj);
}

template <>
inline library::ComponentPrefix deserialize(const SExpression& sexpr,
                                            const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return library::ComponentPrefix(sexpr.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream,
                               const library::ComponentPrefix& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const library::ComponentPrefix& obj) {
  stream << QString("ComponentPrefix('%1'')").arg(*obj);
  return stream;
}

inline uint qHash(const library::ComponentPrefix& key, uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_COMPONENTPREFIX_H
