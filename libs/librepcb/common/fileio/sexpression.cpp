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
#include "sexpression.h"

#include <sexpresso/sexpresso.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SExpression::SExpression() noexcept : mType(Type::String) {
}

SExpression::SExpression(Type type, const QString& value)
  : mType(type), mValue(value) {
}

SExpression::SExpression(const SExpression& other) noexcept
  : mType(other.mType),
    mValue(other.mValue),
    mChildren(other.mChildren),
    mFilePath(other.mFilePath) {
}

SExpression::SExpression(sexpresso::Sexp& sexp, const FilePath& filePath)
  : mType(Type::List), mValue(), mFilePath(filePath) {
  if (sexp.childCount() < 1) {
    throw RuntimeError(__FILE__, __LINE__);
  }

  if (sexp.isSexp()) {
    sexpresso::Sexp& first = sexp.getChild(0);
    if (!first.isString()) {
      throw RuntimeError(__FILE__, __LINE__);
    }
    mValue = QString::fromStdString(first.getString());
    for (auto&& arg : sexp.arguments()) {
      mChildren.append(SExpression(arg, filePath));
    }
  } else if (sexp.isString()) {
    mValue = QString::fromStdString(sexp.getString());
    mType  = Type::String;
  } else {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         tr("Unknown node type."));
  }
}

SExpression::~SExpression() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SExpression::isMultiLineList() const noexcept {
  foreach (const SExpression& child, mChildren) {
    if (child.isLineBreak() || (child.isMultiLineList())) {
      return true;
    }
  }
  return false;
}

const QString& SExpression::getName() const {
  if (isList()) {
    return mValue;
  } else {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         tr("Node is not a list."));
  }
}

const QString& SExpression::getStringOrToken(bool throwIfEmpty) const {
  if (!isToken() && !isString()) {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, mValue,
                         tr("Node is not a token or string."));
  }
  if (mValue.isEmpty() && throwIfEmpty) {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, mValue,
                         tr("Node value is empty."));
  }
  return mValue;
}

QList<SExpression> SExpression::getChildren(const QString& name) const
    noexcept {
  QList<SExpression> children;
  foreach (const SExpression& child, mChildren) {
    if (child.isList() && (child.mValue == name)) {
      children.append(child);
    }
  }
  return children;
}

const SExpression& SExpression::getChildByIndex(int index) const {
  if ((index < 0) || index >= mChildren.count()) {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         tr("Child not found: %1").arg(index));
  }
  return mChildren.at(index);
}

const SExpression* SExpression::tryGetChildByPath(const QString& path) const
    noexcept {
  const SExpression* child = this;
  foreach (const QString& name, path.split('/')) {
    bool found = false;
    foreach (const SExpression& childchild, child->mChildren) {
      if (childchild.isList() && (childchild.mValue == name)) {
        child = &childchild;
        found = true;
      }
    }
    if (!found) {
      return nullptr;
    }
  }
  return child;
}

const SExpression& SExpression::getChildByPath(const QString& path) const {
  const SExpression* child = tryGetChildByPath(path);
  if (child) {
    return *child;
  } else {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         tr("Child not found: %1").arg(path));
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SExpression& SExpression::appendLineBreak() {
  mChildren.append(createLineBreak());
  return *this;
}

SExpression& SExpression::appendList(const QString& name, bool linebreak) {
  return appendChild(createList(name), linebreak);
}

SExpression& SExpression::appendChild(const SExpression& child,
                                      bool               linebreak) {
  if (mType == Type::List) {
    if (linebreak) appendLineBreak();
    mChildren.append(child);
    return mChildren.last();
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

void SExpression::removeLineBreaks() noexcept {
  for (int i = mChildren.count() - 1; i >= 0; --i) {
    if (mChildren.at(i).isLineBreak()) {
      mChildren.removeAt(i);
    }
  }
}

QByteArray SExpression::toByteArray() const {
  QString str = toString(0);  // can throw
  str += '\n';                // newline at end of file
  return str.toUtf8();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

SExpression& SExpression::operator=(const SExpression& rhs) noexcept {
  mType     = rhs.mType;
  mValue    = rhs.mValue;
  mChildren = rhs.mChildren;
  mFilePath = rhs.mFilePath;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString SExpression::escapeString(const QString& string) const noexcept {
  return QString::fromStdString(sexpresso::escape(string.toStdString()));
}

bool SExpression::isValidListName(const QString& name) const noexcept {
  return QRegExp("[a-z][a-z0-9_]*").exactMatch(name);
}

bool SExpression::isValidToken(const QString& token) const noexcept {
  return QRegExp("[a-zA-Z0-9\\.:_-]+").exactMatch(token);
}

QString SExpression::toString(int indent) const {
  if (mType == Type::List) {
    if (!isValidListName(mValue)) {
      throw LogicError(__FILE__, __LINE__,
                       tr("Invalid S-Expression list name: %1").arg(mValue));
    }
    QString str = '(' + mValue;
    for (int i = 0; i < mChildren.count(); ++i) {
      const SExpression& child = mChildren.at(i);
      if ((!str.at(str.length() - 1).isSpace()) && (!child.isLineBreak())) {
        str += ' ';
      }
      bool nextChildIsLineBreak = (i < mChildren.count() - 1)
                                      ? mChildren.at(i + 1).isLineBreak()
                                      : true;
      if (child.isLineBreak() && nextChildIsLineBreak) {
        if ((i > 0) && mChildren.at(i - 1).isLineBreak()) {
          // too many line breaks ;)
        } else {
          str += '\n';
        }
      } else {
        str += child.toString(indent + 1);
      }
    }
    if (isMultiLineList()) {
      str += '\n' + QString(' ').repeated(indent);
    }
    return str + ')';
  } else if (mType == Type::Token) {
    if (!isValidToken(mValue)) {
      throw LogicError(__FILE__, __LINE__,
                       tr("Invalid S-Expression token: %1").arg(mValue));
    }
    return mValue;
  } else if (mType == Type::String) {
    return '"' + escapeString(mValue) + '"';
  } else if (mType == Type::LineBreak) {
    return '\n' + QString(' ').repeated(indent);
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

SExpression SExpression::createList(const QString& name) {
  return SExpression(Type::List, name);
}

SExpression SExpression::createToken(const QString& token) {
  return SExpression(Type::Token, token);
}

SExpression SExpression::createString(const QString& string) {
  return SExpression(Type::String, string);
}

SExpression SExpression::createLineBreak() {
  return SExpression(Type::LineBreak, QString());
}

SExpression SExpression::parse(const QByteArray& content,
                               const FilePath&   filePath) {
  std::string     error;
  QString         str  = QString::fromUtf8(content);
  sexpresso::Sexp tree = sexpresso::parse(str.toStdString(), error);
  if (error.empty()) {
    if (tree.childCount() == 1) {
      return SExpression(tree.getChild(0), filePath);
    } else {
      throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                           tr("File does not have exactly one root node."));
    }
  } else {
    throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                         QString::fromStdString(error));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
