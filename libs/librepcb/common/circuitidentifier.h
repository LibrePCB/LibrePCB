/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_CIRCUITIDENTIFIER_H
#define LIBREPCB_CIRCUITIDENTIFIER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileio/sexpression.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class CircuitIdentifier
 ******************************************************************************/

struct CircuitIdentifierVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(__FILE__, __LINE__,
                                        QString(QApplication::translate(
                                                    "CircuitIdentifier",
                                                    "Invalid identifier: '%1'"))
                                            .arg(val)),
                     std::forward<Value>(val));
  }
};

struct CircuitIdentifierConstraint {
  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("^[-a-zA-Z0-9_+/!?@#$]{1,32}$")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * CircuitIdentifier is a wrapper around QString which guarantees to contain a
 * valid identifier used in circuits.
 *
 * Circuit identifiers can be used for net names, component names or similar
 * things. Such identifiers may be used in SPICE netlist exports, Gerber exports
 * and so on. Because such file formats have pretty strict requirements on
 * identifiers, we use similar restrictions in LibrePCB.
 *
 * An circuit identifier is considered as valid if it:
 *   - contains minimum 1 and maximum 32 characters
 *   - contains only the characters [-a-zA-Z0-9_+/!?@#$]
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid circuit identifier according these rules.
 */
using CircuitIdentifier =
    type_safe::constrained_type<QString, CircuitIdentifierConstraint,
                                CircuitIdentifierVerifier>;

inline bool operator==(const CircuitIdentifier& lhs,
                       const QString&           rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString&           lhs,
                       const CircuitIdentifier& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const CircuitIdentifier& lhs,
                       const QString&           rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString&           lhs,
                       const CircuitIdentifier& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const CircuitIdentifier& lhs,
                         const QString&           rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString&           lhs,
                         const CircuitIdentifier& rhs) noexcept {
  return lhs % (*rhs);
}
inline QString operator%(const CircuitIdentifier& lhs,
                         const CircuitIdentifier& rhs) noexcept {
  return (*lhs) % (*rhs);
}

template <>
inline SExpression serializeToSExpression(const CircuitIdentifier& obj) {
  return SExpression::createString(*obj);
}

template <>
inline CircuitIdentifier deserializeFromSExpression(const SExpression& sexpr,
                                                    bool throwIfEmpty) {
  QString str = sexpr.getStringOrToken(throwIfEmpty);
  // backward compatibility - remove this some time!
  str.remove(QRegularExpression("[^-a-zA-Z0-9_+/!?@#$]"));
  str.truncate(32);
  return CircuitIdentifier(str);  // can throw
}

inline QDataStream& operator<<(QDataStream&             stream,
                               const CircuitIdentifier& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const CircuitIdentifier& obj) {
  stream << QString("CircuitIdentifier('%1'')").arg(*obj);
  return stream;
}

inline uint qHash(const CircuitIdentifier& key, uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_CIRCUITIDENTIFIER_H
