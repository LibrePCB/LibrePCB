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

#ifndef LIBREPCB_GRAPHICSLAYERNAME_H
#define LIBREPCB_GRAPHICSLAYERNAME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class GraphicsLayerName
 ******************************************************************************/

struct GraphicsLayerNameVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(
                         __FILE__, __LINE__,
                         QString(QApplication::translate(
                                     "GraphicsLayerName",
                                     "Not a valid graphics layer name: '%1'"))
                             .arg(val)),
                     std::forward<Value>(val));
  }
};

struct GraphicsLayerNameConstraint {
  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("\\A[a-z][_0-9a-z]{0,39}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * GraphicsLayerName is a wrapper around QString which guarantees to contain a
 * valid name for librepcb::GraphicsLayer
 *
 * A string is considered as valid graphics layer name if:
 *   - it contains minimum 1 and maximum 40 characters
 *   - the first character is one of [a-z] (lowercase)
 *   - the following characters are [a-z] (lowercase), [0-9] or [_] (underscore)
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid graphics layer name according these rules.
 */
using GraphicsLayerName =
    type_safe::constrained_type<QString, GraphicsLayerNameConstraint,
                                GraphicsLayerNameVerifier>;

inline bool operator==(const GraphicsLayerName& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs,
                       const GraphicsLayerName& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const GraphicsLayerName& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs,
                       const GraphicsLayerName& rhs) noexcept {
  return lhs != (*rhs);
}

template <>
inline SExpression serialize(const GraphicsLayerName& obj) {
  return SExpression::createToken(*obj);
}

template <>
inline GraphicsLayerName deserialize(const SExpression& sexpr) {
  return GraphicsLayerName(sexpr.getStringOrToken());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream,
                               const GraphicsLayerName& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const GraphicsLayerName& obj) {
  stream << QString("GraphicsLayerName('%1'')").arg(*obj);
  return stream;
}

inline uint qHash(const GraphicsLayerName& key, uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GRAPHICSLAYERNAME_H
