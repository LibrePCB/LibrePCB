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

#ifndef LIBREPCB_CORE_TAG_H
#define LIBREPCB_CORE_TAG_H

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
 *  Class Tag
 ******************************************************************************/

struct TagVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(
               __FILE__, __LINE__,
               QString(QCoreApplication::translate("Tag", "Invalid tag: '%1'"))
                   .arg(val)),
           std::forward<Value>(val));
  }
};

struct TagConstraint {
  static const constexpr int MAX_LENGTH = 32;

  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("\\A[-a-z0-9.]{1,32}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * Tag is a wrapper around QString which guarantees to contain a valid tag
 *
 * Tags are kebab-case strings (e.g. "ipc-density-level-a") which can be
 * attached to various objects through LibrePCB.
 *
 * An tag is considered as valid if it:
 *   - contains minimum 1 and maximum 32 characters
 *   - contains only the characters [-a-z0-9.]
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid tag according these rules.
 */
using Tag = type_safe::constrained_type<QString, TagConstraint, TagVerifier>;

inline bool operator==(const Tag& lhs, const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs, const Tag& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const Tag& lhs, const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs, const Tag& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const Tag& lhs, const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs, const Tag& rhs) noexcept {
  return lhs % (*rhs);
}
inline QString operator%(const Tag& lhs, const Tag& rhs) noexcept {
  return (*lhs) % (*rhs);
}

template <>
inline std::unique_ptr<SExpression> serialize(const Tag& obj) {
  return SExpression::createString(*obj);
}

template <>
inline Tag deserialize(const SExpression& node) {
  return Tag(node.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const Tag& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const Tag& obj) {
  stream << QString("Tag('%1')").arg(*obj);
  return stream;
}

inline std::size_t qHash(const Tag& key, std::size_t seed = 0) noexcept {
  return ::qHash(*key, seed);
}

inline static QString cleanTag(const QString& userInput) noexcept {
  return Toolbox::cleanUserInputString(
      userInput, QRegularExpression("[^-a-z0-9.]"), true, true, false, "-",
      TagConstraint::MAX_LENGTH);
}

inline static std::optional<Tag> parseTag(const QString& tag) noexcept {
  return TagConstraint()(tag) ? Tag(tag) : std::optional<Tag>();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
