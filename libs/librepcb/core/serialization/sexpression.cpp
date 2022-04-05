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

#include "../application.h"

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

SExpression::~SExpression() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QString& SExpression::getName() const {
  if (isList()) {
    return mValue;
  } else {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         "Node is not a list.");
  }
}

const QString& SExpression::getValue() const {
  if (!isToken() && !isString()) {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, mValue,
                         "Node is not a token or string.");
  }
  return mValue;
}

QList<SExpression> SExpression::getChildren(Type type) const noexcept {
  QList<SExpression> children;
  foreach (const SExpression& child, mChildren) {
    if (child.getType() == type) {
      children.append(child);
    }
  }
  return children;
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

const SExpression& SExpression::getChild(const QString& path) const {
  const SExpression* child = tryGetChild(path);
  if (child) {
    return *child;
  } else {
    throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                         QString("Child not found: %1").arg(path));
  }
}

const SExpression* SExpression::tryGetChild(const QString& path) const
    noexcept {
  const SExpression* child = this;
  foreach (const QString& name, path.split('/')) {
    if (name.startsWith('@')) {
      bool valid = false;
      int index = name.mid(1).toInt(&valid);
      if ((valid) && (index >= 0) && skipLineBreaks(child->mChildren, index)) {
        child = &child->mChildren.at(index);
      } else {
        return nullptr;
      }
    } else {
      bool found = false;
      foreach (const SExpression& childchild, child->mChildren) {
        if (childchild.isList() && (childchild.mValue == name)) {
          child = &childchild;
          found = true;
          break;
        }
      }
      if (!found) {
        return nullptr;
      }
    }
  }
  return child;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SExpression::ensureLineBreak() {
  if (mChildren.isEmpty() || (!mChildren.last().isLineBreak())) {
    mChildren.append(createLineBreak());
  }
}

void SExpression::ensureLineBreakIfMultiLine() {
  // It's ugly to conditionally create line breaks, let's always create them
  // for file format v0.2.
  if ((!legacyMode()) || isMultiLine()) {
    ensureLineBreak();
  }
}

void SExpression::ensureEmptyLine() {
  // Empty lines are cumbersome, let's stop creating them for file format v0.2.
  if (legacyMode()) {
    while ((mChildren.count() < 2) || (!mChildren.last().isLineBreak()) ||
           (!mChildren.at(mChildren.count() - 2).isLineBreak())) {
      mChildren.append(createLineBreak());
    }
  } else {
    ensureLineBreak();
  }
}

SExpression& SExpression::appendList(const QString& name) {
  return appendChild(createList(name));
}

SExpression& SExpression::appendChild(const SExpression& child) {
  if (mType == Type::List) {
    mChildren.append(child);
    return mChildren.last();
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

QByteArray SExpression::toByteArray() const {
  QString str = toString(0);  // can throw
  if (!str.endsWith('\n')) {
    str += '\n';  // newline at end of file
  }
  return str.toUtf8();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

SExpression& SExpression::operator=(const SExpression& rhs) noexcept {
  mType = rhs.mType;
  mValue = rhs.mValue;
  mChildren = rhs.mChildren;
  mFilePath = rhs.mFilePath;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString SExpression::escapeString(const QString& string) noexcept {
  static QHash<QChar, QString> replacements;
  if (replacements.isEmpty()) {
    replacements = {
        {'"', "\\\""},  // Double quote *must* be escaped
        {'\\', "\\\\"},  // Backslash *must* be escaped
        {'\b', "\\b"},  // Escape backspace to increase readability
        {'\f', "\\f"},  // Escape form feed to increase readability
        {'\n', "\\n"},  // Escape line feed to increase readability
        {'\r', "\\r"},  // Escape carriage return to increase readability
        {'\t', "\\t"},  // Escape horizontal tab to increase readability
        {'\v', "\\v"},  // Escape vertical tab to increase readability
    };
    if (qApp->getFileFormatVersion() < Version::fromString("0.2")) {
      // Until LibrePCB 0.1.5 we used sexpresso::escape() to escape strings.
      // This function escaped more characters than actually needed. To avoid
      // modifying the file format in LibrePCB 0.1.6, we emulate the same
      // escaping behavior. In LibrePCB 0.2.x we are allowed to modify the file
      // format, so let's get rid of these legacy escaping behavior.
      replacements.insert('\'', "\\\'");  // Single quote
      replacements.insert('\?', "\\?");  // Question mark
      replacements.insert('\a', "\\a");  // Audible bell
    }
  }

  QString escaped;
  escaped.reserve(string.length() + (string.length() / 10));
  foreach (const QChar& c, string) { escaped += replacements.value(c, c); }
  return escaped;
}

bool SExpression::isValidToken(const QString& token) noexcept {
  if (token.isEmpty()) {
    return false;
  }
  foreach (const QChar& c, token) {
    if (!isValidTokenChar(c)) {
      return false;
    }
  }
  return true;
}

bool SExpression::isValidTokenChar(const QChar& c) noexcept {
  static QSet<QChar> allowedSpecialChars = {'\\', '.', ':', '_', '-'};
  return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
      ((c >= '0') && (c <= '9')) || allowedSpecialChars.contains(c);
}

QString SExpression::toString(int indent) const {
  if (mType == Type::List) {
    if (!isValidToken(mValue)) {
      throw LogicError(
          __FILE__, __LINE__,
          QString("Invalid S-Expression list name: %1").arg(mValue));
    }
    QString str = '(' + mValue;
    bool lastCharIsSpace = false;
    const int lastIndex = mChildren.count() - 1;
    for (int i = 0; i < mChildren.count(); ++i) {
      const SExpression& child = mChildren.at(i);
      if ((!lastCharIsSpace) && (!child.isLineBreak())) {
        str += ' ';
      }
      const bool nextChildIsLineBreak =
          (i < lastIndex) && mChildren.at(i + 1).isLineBreak();
      int currentIndent =
          (child.isLineBreak() && nextChildIsLineBreak) ? 0 : (indent + 1);
      lastCharIsSpace = child.isLineBreak() && (currentIndent > 0);
      if (lastCharIsSpace && (i == lastIndex)) {
        --currentIndent;
      }
      str += child.toString(currentIndent);
    }
    return str + ')';
  } else if (mType == Type::Token) {
    if (!isValidToken(mValue)) {
      throw LogicError(__FILE__, __LINE__,
                       QString("Invalid S-Expression token: %1").arg(mValue));
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
                               const FilePath& filePath) {
  int index = 0;
  QString contentStr = QString::fromUtf8(content);
  skipWhitespaceAndComments(contentStr, index, true);  // Skip newlines as well.
  if (index >= contentStr.length()) {
    throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                         "No S-Expression node found.");
  }
  SExpression root = parse(contentStr, index, filePath);
  skipWhitespaceAndComments(contentStr, index, true);  // Skip newlines as well.
  if (index < contentStr.length()) {
    throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                         "File contains more than one root node.");
  }
  return root;
}

bool& SExpression::legacyMode() noexcept {
  static bool v01 = (qApp->getFileFormatVersion() < Version::fromString("0.2"));
  return v01;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SExpression::isMultiLine() const noexcept {
  if (isLineBreak()) {
    return true;
  } else if (isList()) {
    foreach (const SExpression& child, mChildren) {
      if (child.isMultiLine()) {
        return true;
      }
    }
  }
  return false;
}

bool SExpression::skipLineBreaks(const QList<SExpression>& children,
                                 int& index) noexcept {
  for (int i = 0; i < children.count(); ++i) {
    if (children.at(i).isLineBreak()) {
      ++index;
    } else if (i == index) {
      return true;
    }
  }
  return false;
}

SExpression SExpression::parse(const QString& content, int& index,
                               const FilePath& filePath) {
  Q_ASSERT(index < content.length());

  if (content.at(index) == '\n') {
    ++index;  // consume the '\n'
    skipWhitespaceAndComments(content, index);  // consume following spaces
    return createLineBreak();
  } else if (content.at(index) == '(') {
    return parseList(content, index, filePath);
  } else if (content.at(index) == '"') {
    return createString(parseString(content, index, filePath));
  } else {
    return createToken(parseToken(content, index, filePath));
  }
}

SExpression SExpression::parseList(const QString& content, int& index,
                                   const FilePath& filePath) {
  Q_ASSERT((index < content.length()) && (content.at(index) == '('));

  ++index;  // consume the '('

  SExpression list = createList(parseToken(content, index, filePath));

  while (true) {
    if (index >= content.length()) {
      throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                           "S-Expression node ended without closing ')'.");
    }
    if (content.at(index) == ')') {
      ++index;  // consume the ')'
      skipWhitespaceAndComments(content, index);  // consume following spaces
      break;
    } else {
      list.appendChild(parse(content, index, filePath));
    }
  }

  return list;
}

QString SExpression::parseToken(const QString& content, int& index,
                                const FilePath& filePath) {
  int oldIndex = index;
  while ((index < content.length()) && (isValidTokenChar(content.at(index)))) {
    ++index;
  }
  QString token = content.mid(oldIndex, index - oldIndex);
  if (token.isEmpty()) {
    throw FileParseError(
        __FILE__, __LINE__, filePath, -1, -1, QString(),
        QString("Invalid token character detected: '%1'")
            .arg(index < content.length() ? content.at(index) : QChar()));
  }
  skipWhitespaceAndComments(content, index);  // consume following spaces
  return token;
}

QString SExpression::parseString(const QString& content, int& index,
                                 const FilePath& filePath) {
  ++index;  // consume the '"'

  // Note: Until LibrePCB 0.1.5 we used the sexpresso library for escaping
  // strings. This library escaped more characters than we do now. To still
  // support reading the file format 0.1, we have to keep support for the
  // old escaping behavior.
  static QHash<QChar, QChar> escapedChars = {
      {'\'', '\''},  // Single quote
      {'"', '"'},  // Double quote
      {'?', '\?'},  // Question mark
      {'\\', '\\'},  // Backslash
      {'a', '\a'},  // Audible bell
      {'b', '\b'},  // Backspace
      {'f', '\f'},  // Form feed
      {'n', '\n'},  // Line feed
      {'r', '\r'},  // Carriage return
      {'t', '\t'},  // Horizontal tab
      {'v', '\v'},  // Vertical tab
  };

  QString string;
  bool escaped = false;
  while (true) {
    if (index >= content.length()) {
      throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                           "String ended without quote.");
    }
    const QChar& c = content.at(index);
    if (escaped) {
      if (escapedChars.contains(c)) {
        string += escapedChars[c];
        ++index;
        escaped = false;
      } else {
        throw FileParseError(__FILE__, __LINE__, filePath, -1, -1, QString(),
                             QString("Illegal escape sequence: '\\%1'").arg(c));
      }
    } else if (c == '"') {
      ++index;  // consume the '"'
      skipWhitespaceAndComments(content, index);  // consume following spaces
      break;
    } else if (c == '\\') {
      escaped = true;
      ++index;
    } else {
      string += c;
      ++index;
    }
  }
  return string;
}

void SExpression::skipWhitespaceAndComments(const QString& content, int& index,
                                            bool skipNewline) {
  static QSet<QChar> spaces = {' ', '\f', '\r', '\t', '\v'};

  bool isComment = false;
  while (index < content.length()) {
    const QChar& c = content.at(index);
    if (c == ';') {  // Line-comment of the Lisp language
      isComment = true;
    } else if (c == '\n') {
      isComment = false;
    }
    if (isComment || ((skipNewline) && (c == '\n')) || spaces.contains(c)) {
      ++index;
    } else {
      break;
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
