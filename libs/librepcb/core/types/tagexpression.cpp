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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "tagexpression.h"

#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const TagExpression& obj) {
  return SExpression::createString(obj.toString());
}

template <>
TagExpression deserialize(const SExpression& node) {
  return TagExpression::fromString(node.getValue());
}

template <>
std::unique_ptr<SExpression> serialize(const TagConditional& obj) {
  std::unique_ptr<SExpression> node = SExpression::createList("preferred_tags");
  obj.serialize(*node);
  return node;
}

template <>
TagConditional deserialize(const SExpression& node) {
  return TagConditional(node);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TagExpression::TagExpression(const Tag& tag, bool builtIn,
                             bool inverted) noexcept
  : mTag(tag), mBuiltIn(builtIn), mInverted(inverted) {
}

TagExpression::TagExpression(const TagExpression& other) noexcept
  : mTag(other.mTag), mBuiltIn(other.mBuiltIn), mInverted(other.mInverted) {
}

TagExpression::~TagExpression() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString TagExpression::toString() const noexcept {
  QString s;
  if (mInverted) {
    s += "!";
  }
  if (mBuiltIn) {
    s += "#";
  }
  return s + *mTag;
}

TagExpression TagExpression::fromString(QString s) {
  bool inverted = false;
  bool builtIn = false;
  if (s.startsWith('!')) {
    inverted = true;
    s.remove(0, 1);
  }
  if (s.startsWith('#')) {
    builtIn = true;
    s.remove(0, 1);
  }
  return TagExpression(Tag(s), builtIn, inverted);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool TagExpression::operator==(const TagExpression& rhs) const noexcept {
  return (mTag == rhs.mTag) && (mBuiltIn == rhs.mBuiltIn) &&
      (mInverted == rhs.mInverted);
}

TagExpression& TagExpression::operator=(const TagExpression& rhs) noexcept {
  mTag = rhs.mTag;
  mBuiltIn = rhs.mBuiltIn;
  mInverted = rhs.mInverted;
  return *this;
}

/*******************************************************************************
 *  Class TagConditional
 ******************************************************************************/

TagConditional::TagConditional(const QVector<TagExpression>& expressions)
  : mExpressions(expressions) {
  if (mExpressions.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Tag conditional must not be empty.");
  }
}

TagConditional::TagConditional(const SExpression& node) : mExpressions() {
  for (const SExpression* child : node.getChildren(SExpression::Type::String)) {
    mExpressions.append(deserialize<TagExpression>(*child));
  }
  if (mExpressions.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Tag conditional must not be empty.");
  }
}

TagConditional::TagConditional(const TagConditional& other) noexcept
  : mExpressions(other.mExpressions) {
}

TagConditional::~TagConditional() noexcept {
}

QString TagConditional::toString() const noexcept {
  QStringList items;
  for (const TagExpression& expr : mExpressions) {
    items.append(expr.toString());
  }
  return items.join(" && ");
}

bool TagConditional::matches(const QSet<Tag>& builtIn,
                             const QSet<Tag>& userDefined) const noexcept {
  for (const TagExpression& expr : mExpressions) {
    const QSet<Tag>& input = expr.isBuiltIn() ? builtIn : userDefined;
    if (expr.isInverted() != input.contains(expr.getTag())) {
      return false;
    }
  }
  return true;
}

void TagConditional::serialize(SExpression& root) const {
  for (const TagExpression& e : mExpressions) {
    root.appendChild(e);
  }
}

bool TagConditional::operator==(const TagConditional& rhs) const noexcept {
  return mExpressions == rhs.mExpressions;
}

TagConditional& TagConditional::operator=(const TagConditional& rhs) noexcept {
  mExpressions = rhs.mExpressions;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
