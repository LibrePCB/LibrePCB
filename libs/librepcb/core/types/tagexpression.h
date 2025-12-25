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

#ifndef LIBREPCB_CORE_TAGEXPRESSION_H
#define LIBREPCB_CORE_TAGEXPRESSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "tag.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TagExpression
 ******************************************************************************/

/**
 * @brief Represents an expression for tag matching (e.g. "!#smt")
 *
 * Extends ::librepcb::Tag with two features:
 *  - Inversion (denoted by '!' prefix)
 *  - Built-in tag notation (denoted by '#' prefix)
 */
class TagExpression final {
  Q_DECLARE_TR_FUNCTIONS(TagExpression)

public:
  // Constructors / Destructor
  TagExpression() = delete;
  TagExpression(const Tag& tag, bool builtIn, bool inverted) noexcept;
  TagExpression(const TagExpression& other) noexcept;
  ~TagExpression() noexcept;

  // Getters
  const Tag& getTag() const noexcept { return mTag; }
  bool isBuiltIn() const noexcept { return mBuiltIn; }
  bool isInverted() const noexcept { return mInverted; }

  // General Methods
  QString toString() const noexcept;
  static TagExpression fromString(QString s);

  // Operator Overloadings
  bool operator==(const TagExpression& rhs) const noexcept;
  bool operator!=(const TagExpression& rhs) const noexcept {
    return !(*this == rhs);
  }
  TagExpression& operator=(const TagExpression& rhs) noexcept;

private:  // Data
  Tag mTag;
  bool mBuiltIn;
  bool mInverted;
};

/*******************************************************************************
 *  Class TagConditional
 ******************************************************************************/

/**
 * @brief A list of multiple ::librepcb::TagExpression objects
 */
class TagConditional final {
  Q_DECLARE_TR_FUNCTIONS(TagConditional)

public:
  // Constructors / Destructor
  TagConditional() = delete;
  explicit TagConditional(const QVector<TagExpression>& expressions);
  explicit TagConditional(const SExpression& node);
  TagConditional(const TagConditional& other) noexcept;
  ~TagConditional() noexcept;

  // Getters
  const QVector<TagExpression>& getExpressions() const noexcept {
    return mExpressions;
  }
  QString toString() const noexcept;

  // General Methods
  bool matches(const QSet<Tag>& builtIn,
               const QSet<Tag>& userDefined) const noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const TagConditional& rhs) const noexcept;
  bool operator!=(const TagConditional& rhs) const noexcept {
    return !(*this == rhs);
  }
  TagConditional& operator=(const TagConditional& rhs) noexcept;

private:  // Data
  QVector<TagExpression> mExpressions;  ///< Minimum 1 item!
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
