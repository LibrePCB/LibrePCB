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

#ifndef LIBREPCB_CORE_SIMPLESTRING_H
#define LIBREPCB_CORE_SIMPLESTRING_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../qtcompat.h"
#include "../serialization/sexpression.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SimpleString
 ******************************************************************************/

struct SimpleStringVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(__FILE__, __LINE__,
                              QString(QCoreApplication::translate(
                                          "SimpleString", "Invalid name: '%1'"))
                                  .arg(val)),
           std::forward<Value>(val));
  }
};

struct SimpleStringConstraint {
  bool operator()(const QString& value) const noexcept {
    if (value != value.trimmed()) return false;
    foreach (const QChar& c, value) {
      if (!c.isPrint()) return false;
    }
    return true;
  }
};

/**
 * SimpleString is a wrapper around QString which guarantees to contain a
 * string
 *
 * A string is considered simple if it only consists of printable characters
 * and doesn't contain leading or trailing spaces. Note that an empty string
 * is allowed.
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid simple string.
 */
using SimpleString =
    type_safe::constrained_type<QString, SimpleStringConstraint,
                                SimpleStringVerifier>;

inline bool operator==(const SimpleString& lhs, const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs, const SimpleString& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const SimpleString& lhs, const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs, const SimpleString& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const SimpleString& lhs, const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs, const SimpleString& rhs) noexcept {
  return lhs % (*rhs);
}
inline SimpleString operator%(const SimpleString& lhs,
                              const SimpleString& rhs) noexcept {
  return SimpleString((*lhs) % (*rhs));  // always safe, will not throw
}

inline static SimpleString cleanSimpleString(
    const QString& userInput) noexcept {
  QString ret = userInput.simplified();
  for (int i = ret.length() - 1; i >= 0; --i) {
    if (!ret[i].isPrint()) {
      ret.remove(i, 1);
    }
  }
  return SimpleString(ret);
}

template <>
inline std::unique_ptr<SExpression> serialize(const SimpleString& obj) {
  return SExpression::createString(*obj);
}

template <>
inline SimpleString deserialize(const SExpression& node) {
  return SimpleString(node.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const SimpleString& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const SimpleString& obj) {
  stream << QString("SimpleString('%1')").arg(*obj);
  return stream;
}

inline QtCompat::Hash qHash(const SimpleString& key,
                            QtCompat::Hash seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
