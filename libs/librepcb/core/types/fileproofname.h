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

#ifndef LIBREPCB_CORE_FILEPROOFNAME_H
#define LIBREPCB_CORE_FILEPROOFNAME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../qtcompat.h"
#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class FileProofName
 ******************************************************************************/

inline static QString cleanFileProofName(const QString& userInput) noexcept {
  return Toolbox::cleanUserInputString(userInput,
                                       QRegularExpression("[^-a-zA-Z0-9_+().]"),
                                       true, false, false, "-", 20);
}

struct FileProofNameVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(
                         __FILE__, __LINE__,
                         QString(QCoreApplication::translate(
                                     "FileProofName", "Invalid name: '%1'"))
                             .arg(val)),
                     std::forward<Value>(val));
  }
};

struct FileProofNameConstraint {
  static QRegularExpression regex() noexcept {
    return QRegularExpression("\\A[-a-zA-Z0-9_+().]{1,20}\\z");
  }

  bool operator()(const QString& value) const noexcept {
    return regex()
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * FileProofName is a wrapper around QString which guarantees to contain a
 * string useble within filenames
 *
 * A sttring name is considered file-proof if it:
 *   - contains minimum 1 and maximum 20 characters
 *   - contains only the characters [-a-zA-Z0-9_+().]
 *
 * The constructor throws an exception if constructed from a QString which is
 * not file-proof according these rules.
 */
using FileProofName =
    type_safe::constrained_type<QString, FileProofNameConstraint,
                                FileProofNameVerifier>;

inline bool operator==(const FileProofName& lhs, const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs, const FileProofName& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const FileProofName& lhs, const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs, const FileProofName& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const FileProofName& lhs,
                         const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs,
                         const FileProofName& rhs) noexcept {
  return lhs % (*rhs);
}
inline FileProofName operator%(const FileProofName& lhs,
                               const FileProofName& rhs) noexcept {
  return FileProofName((*lhs) % (*rhs));  // always safe, will not throw
}

template <>
inline std::unique_ptr<SExpression> serialize(const FileProofName& obj) {
  return SExpression::createString(*obj);
}

template <>
inline FileProofName deserialize(const SExpression& node) {
  return FileProofName(node.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const FileProofName& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream, const FileProofName& obj) {
  stream << QString("FileProofName('%1')").arg(*obj);
  return stream;
}

inline QtCompat::Hash qHash(const FileProofName& key,
                            QtCompat::Hash seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
