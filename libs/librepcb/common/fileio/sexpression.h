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

#ifndef LIBREPCB_SEXPRESSION_H
#define LIBREPCB_SEXPRESSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "filepath.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/**
 * Serialize an object to a ::librepcb::SExpression
 *
 * @tparam T    Type of object to serialize.
 * @param obj   Object to serialize.
 * @return      Serialized S-Expression.
 * @throws      ::librepcb::Exception in case of an error.
 */
template <typename T>
SExpression serialize(const T& obj);

/**
 * Deserialize an ::librepcb::SExpression to an object
 *
 * @tparam T    Type of object to deserialize.
 * @param sexpr S-Expression to deserialize.
 * @return      Deserialized object.
 * @throws      ::librepcb::Exception in case of an error.
 */
template <typename T>
T deserialize(const SExpression& sexpr);

/*******************************************************************************
 *  Class SExpression
 ******************************************************************************/

/**
 * @brief The SExpression class
 */
class SExpression final {
  Q_DECLARE_TR_FUNCTIONS(SExpression)

public:
  // Types
  enum class Type {
    List,  ///< has a tag name and an arbitrary number of children
    Token,  ///< values without quotes (e.g. `-12.34`)
    String,  ///< values with double quotes (e.g. `"Foo!"`)
    LineBreak,  ///< manual line break inside a List
  };

  // Constructors / Destructor
  SExpression() noexcept;
  SExpression(const SExpression& other) noexcept;
  ~SExpression() noexcept;

  // Getters
  const FilePath& getFilePath() const noexcept { return mFilePath; }
  Type getType() const noexcept { return mType; }
  bool isList() const noexcept { return mType == Type::List; }
  bool isToken() const noexcept { return mType == Type::Token; }
  bool isString() const noexcept { return mType == Type::String; }
  bool isLineBreak() const noexcept { return mType == Type::LineBreak; }
  bool isMultiLineList() const noexcept;
  const QString& getName() const;
  const QString& getStringOrToken() const;
  const QList<SExpression>& getChildren() const { return mChildren; }
  QList<SExpression> getChildren(const QString& name) const noexcept;
  const SExpression& getChildByIndex(int index) const;
  const SExpression* tryGetChildByPath(const QString& path) const noexcept;
  const SExpression& getChildByPath(const QString& path) const;

  template <typename T>
  T getValue() const {
    try {
      return deserialize<T>(*this);
    } catch (const Exception& e) {
      throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, mValue,
                           e.getMsg());
    }
  }

  template <typename T>
  T getValueByPath(const QString& path) const {
    const SExpression& child = getChildByPath(path);
    return child.getValueOfFirstChild<T>();
  }

  template <typename T>
  T getValueOfFirstChild() const {
    if (mChildren.count() < 1) {
      throw FileParseError(__FILE__, __LINE__, mFilePath, -1, -1, QString(),
                           tr("Node does not have children."));
    }
    return mChildren.at(0).getValue<T>();
  }

  // General Methods
  SExpression& appendLineBreak();
  SExpression& appendList(const QString& name, bool linebreak);
  SExpression& appendChild(const SExpression& child, bool linebreak);
  template <typename T>
  SExpression& appendChild(const T& obj) {
    appendChild(serialize(obj), false);
    return *this;
  }
  template <typename T>
  SExpression& appendChild(const QString& child, const T& obj, bool linebreak) {
    return appendList(child, linebreak).appendChild(obj);
  }
  void removeLineBreaks() noexcept;
  QByteArray toByteArray() const;

  // Operator Overloadings
  SExpression& operator=(const SExpression& rhs) noexcept;

  // Static Methods
  static SExpression createList(const QString& name);
  static SExpression createToken(const QString& token);
  static SExpression createString(const QString& string);
  static SExpression createLineBreak();
  static SExpression parse(const QByteArray& content, const FilePath& filePath);

private:  // Methods
  SExpression(Type type, const QString& value);

  static SExpression parse(const QString& content, int& index,
                           const FilePath& filePath);
  static SExpression parseList(const QString& content, int& index,
                               const FilePath& filePath);
  static QString parseToken(const QString& content, int& index,
                            const FilePath& filePath);
  static QString parseString(const QString& content, int& index,
                             const FilePath& filePath);
  static void skipWhitespaceAndComments(const QString& content, int& index);
  static QString escapeString(const QString& string) noexcept;
  static bool isValidToken(const QString& token) noexcept;
  static bool isValidTokenChar(const QChar& c) noexcept;
  QString toString(int indent) const;

private:  // Data
  Type mType;
  QString mValue;  ///< either a list name, a token or a string
  QList<SExpression> mChildren;
  FilePath mFilePath;
};

/*******************************************************************************
 *  Serialization Methods
 ******************************************************************************/

template <>
inline SExpression serialize(const QString& obj) {
  return SExpression::createString(obj);
}

template <>
inline SExpression serialize(const bool& obj) {
  return SExpression::createToken(obj ? "true" : "false");
}

template <>
inline SExpression serialize(const int& obj) {
  return SExpression::createToken(QString::number(obj));
}

template <>
inline SExpression serialize(const uint& obj) {
  return SExpression::createToken(QString::number(obj));
}

template <>
inline SExpression serialize(const QColor& obj) {
  return SExpression::createString(obj.isValid() ? obj.name(QColor::HexArgb)
                                                 : "");
}

template <>
inline SExpression serialize(const QUrl& obj) {
  return SExpression::createString(
      obj.isValid() ? obj.toString(QUrl::PrettyDecoded) : "");
}

template <>
inline SExpression serialize(const QDateTime& obj) {
  return SExpression::createToken(obj.toUTC().toString(Qt::ISODate));
}

template <>
inline SExpression serialize(const SExpression& obj) {
  return obj;
}

/*******************************************************************************
 *  Deserialization Methods
 ******************************************************************************/

template <>
inline QString deserialize(const SExpression& sexpr) {
  return sexpr.getStringOrToken();  // can throw
}

template <>
inline bool deserialize(const SExpression& sexpr) {
  if (sexpr.getStringOrToken() == "true") {
    return true;
  } else if (sexpr.getStringOrToken() == "false") {
    return false;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid boolean."));
}

template <>
inline int deserialize(const SExpression& sexpr) {
  bool ok = false;
  int value = sexpr.getStringOrToken().toInt(&ok);
  if (ok) {
    return value;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid integer."));
}

template <>
inline uint deserialize(const SExpression& sexpr) {
  bool ok = false;
  uint value = sexpr.getStringOrToken().toUInt(&ok);
  if (ok) {
    return value;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid unsigned integer."));
}

template <>
inline QDateTime deserialize(const SExpression& sexpr) {
  QDateTime obj = QDateTime::fromString(sexpr.getStringOrToken(), Qt::ISODate)
                      .toLocalTime();
  if (obj.isValid())
    return obj;
  else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid datetime."));
}

template <>
inline QColor deserialize(const SExpression& sexpr) {
  QColor obj(sexpr.getStringOrToken());
  if (obj.isValid()) {
    return obj;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid color."));
}

template <>
inline QUrl deserialize(const SExpression& sexpr) {
  QUrl obj(sexpr.getStringOrToken(), QUrl::StrictMode);
  if (obj.isValid()) {
    return obj;
  } else
    throw RuntimeError(__FILE__, __LINE__, SExpression::tr("Not a valid URL."));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SEXPRESSION_H
