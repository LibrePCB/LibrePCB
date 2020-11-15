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
class Version;

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
 * @tparam T          Type of object to deserialize.
 * @param sexpr       S-Expression to deserialize.
 * @param fileFormat  The file format version of the passed S-Expression.
 *                    If this is older than the latest file format version,
 *                    a migration might need to be performed.
 * @return            Deserialized object.
 * @throws            ::librepcb::Exception in case of an error.
 */
template <typename T>
T deserialize(const SExpression& sexpr, const Version& fileFormat);

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
  const QString& getValue() const;
  const QList<SExpression>& getChildren() const noexcept { return mChildren; }
  QList<SExpression> getChildren(const QString& name) const noexcept;

  /**
   * @brief Get a child by path
   *
   * This method allows to get a specific child, even nested child.
   * Consider this S-Expression:
   *
   * @verbatim
   * (netsegment 3115f409-5e6c-4023-a8ab-06428ed0720a
   *  (via 2cc45b07-1bef-4340-9292-b54b011c70c5
   *   (position 35.91989 46.0375) (size 0.7) (drill 0.3) (shape round)
   *  )
   * )
   * @endverbatim
   *
   * - To get the UUID of the net segment, use the path "@0" (first child).
   * - To get the whole "via" element (incl. children), use the path "via".
   * - To get the Y coordinate of the via, use the path "via/position/@1".
   *
   * @attention If there exist several childs with (the begin of) the specified
   *            path, only the first match is returned! So if the example above
   *            had more "via" elements, all after the first one would be
   *            ignored. And for example if the first "via" element had no
   *            "position" child, an exception is raised even if the following
   *            "via" elements do have a "position" child.
   *
   * @param path    The path to the child to get, separated by forward slashes
   *                '/'. To specify a child by index, use '@' followed by the
   *                index (e.g. '@1' to get the second child).
   *
   * @return A reference to the child of the specified path.
   *
   * @throws ::librepcb::Exception if the specified child does not exist.
   */
  const SExpression& getChild(const QString& path) const;

  /**
   * @brief Try get a child by path
   *
   * This is exactly the same as #getChild(), but returns `nullptr` if the
   * specified child does not exist (instead of throwing an exception).
   *
   * @param path    See documentation of #getChild().
   *
   * @return  A pointer to the child of the specified path, if found. If no
   *          such child exists, `nullptr` is returned.
   */
  const SExpression* tryGetChild(const QString& path) const noexcept;

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
inline QString deserialize(const SExpression& sexpr,
                           const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return sexpr.getValue();  // can throw
}

template <>
inline bool deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  if (sexpr.getValue() == "true") {
    return true;
  } else if (sexpr.getValue() == "false") {
    return false;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid boolean."));
}

template <>
inline int deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  bool ok = false;
  int value = sexpr.getValue().toInt(&ok);
  if (ok) {
    return value;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid integer."));
}

template <>
inline uint deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  bool ok = false;
  uint value = sexpr.getValue().toUInt(&ok);
  if (ok) {
    return value;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid unsigned integer."));
}

template <>
inline QDateTime deserialize(const SExpression& sexpr,
                             const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  QDateTime obj =
      QDateTime::fromString(sexpr.getValue(), Qt::ISODate).toLocalTime();
  if (obj.isValid())
    return obj;
  else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid datetime."));
}

template <>
inline QColor deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  QColor obj(sexpr.getValue());
  if (obj.isValid()) {
    return obj;
  } else
    throw RuntimeError(__FILE__, __LINE__,
                       SExpression::tr("Not a valid color."));
}

template <>
inline QUrl deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  QUrl obj(sexpr.getValue(), QUrl::StrictMode);
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
