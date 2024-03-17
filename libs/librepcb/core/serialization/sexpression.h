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

#ifndef LIBREPCB_CORE_SEXPRESSION_H
#define LIBREPCB_CORE_SEXPRESSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"

#include <QtCore>

#include <memory>
#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/**
 * Serialize an object to an ::librepcb::SExpression
 *
 * @tparam T    Type of object to serialize.
 * @param obj   Object to serialize.
 * @return      Serialized S-Expression.
 * @throws      ::librepcb::Exception in case of an error.
 */
template <typename T>
std::unique_ptr<SExpression> serialize(const T& obj);

/**
 * Deserialize an ::librepcb::SExpression node to an object
 *
 * @tparam T          Type of object to deserialize.
 * @param node        S-Expression to deserialize.
 * @return            Deserialized object.
 * @throws            ::librepcb::Exception in case of an error.
 */
template <typename T>
T deserialize(const SExpression& node);

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
  const QString& getName() const;
  const QString& getValue() const;
  std::size_t getChildCount() const noexcept { return mChildren.size(); }
  bool containsChild(const SExpression& child) const noexcept;
  SExpression& getChild(int index);
  const SExpression& getChild(int index) const;
  QList<SExpression*> getChildren(Type type) noexcept;
  QList<const SExpression*> getChildren(Type type) const noexcept;
  QList<SExpression*> getChildren(const QString& name) noexcept;
  QList<const SExpression*> getChildren(const QString& name) const noexcept;

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
   * @note  In contrast to #getChildren(), this method skips linebreak
   *        elements. So if you acces an element by index (e.g. "@3"),
   *        the n-th child which is *not* a linebreak will be returned.
   *
   * @param path    The path to the child to get, separated by forward slashes
   *                '/'. To specify a child by index, use '@' followed by the
   *                index (e.g. '@1' to get the second child).
   *
   * @return A reference to the child of the specified path.
   *
   * @throws ::librepcb::Exception if the specified child does not exist.
   */
  SExpression& getChild(const QString& path);
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
  SExpression* tryGetChild(const QString& path) noexcept;
  const SExpression* tryGetChild(const QString& path) const noexcept;

  // Setters
  void setName(const QString& name);
  void setValue(const QString& value);

  // General Methods
  void ensureLineBreak();
  SExpression& appendList(const QString& name);
  void appendChild(std::unique_ptr<SExpression> child);
  template <typename T>
  SExpression& appendChild(const T& obj) {
    appendChild(serialize(obj));
    return *mChildren.back();
  }
  void appendChild(const QString& child, std::unique_ptr<SExpression> obj) {
    Q_ASSERT(obj);
    appendList(child).appendChild(std::move(obj));
  }
  template <typename T>
  SExpression& appendChild(const QString& child, const T& obj) {
    appendChild(child, serialize(obj));
    return *mChildren.back();
  }
  void removeChild(const SExpression& child);
  void removeChildrenWithNodeRecursive(const SExpression& search) noexcept;
  void replaceRecursive(const SExpression& search,
                        const SExpression& replace) noexcept;
  QByteArray toByteArray() const;

  // Operator Overloadings
  bool operator==(const SExpression& rhs) const noexcept;
  bool operator!=(const SExpression& rhs) const noexcept {
    return !(*this == rhs);
  }
  bool operator<(const SExpression& rhs) const noexcept;
  SExpression& operator=(const SExpression& rhs) noexcept;

  // Static Methods
  static std::unique_ptr<SExpression> createList(const QString& name);
  static std::unique_ptr<SExpression> createToken(const QString& token);
  static std::unique_ptr<SExpression> createString(const QString& string);
  static std::unique_ptr<SExpression> createLineBreak();
  static std::unique_ptr<SExpression> parse(const QByteArray& content,
                                            const FilePath& filePath);

private:  // Methods
  SExpression(Type type, const QString& value);

  bool isMultiLine() const noexcept;
  static bool skipLineBreaks(
      const std::vector<std::unique_ptr<SExpression>>& children,
      int& index) noexcept;
  static std::unique_ptr<SExpression> parse(const QString& content, int& index,
                                            const FilePath& filePath);
  static std::unique_ptr<SExpression> parseList(const QString& content,
                                                int& index,
                                                const FilePath& filePath);
  static QString parseToken(const QString& content, int& index,
                            const FilePath& filePath);
  static QString parseString(const QString& content, int& index,
                             const FilePath& filePath);
  static void skipWhitespaceAndComments(const QString& content, int& index,
                                        bool skipNewline = false);
  static QString escapeString(const QString& string) noexcept;
  static bool isValidToken(const QString& token) noexcept;
  static bool isValidTokenChar(const QChar& c) noexcept;
  QString toString(int indent) const;

private:  // Data
  Type mType;
  QString mValue;  ///< either a list name, a token or a string
  // Note: For memory-safe removal operations we don't use a Qt container class!
  std::vector<std::unique_ptr<SExpression>> mChildren;
  FilePath mFilePath;

  // qHash() needs access to mChildrenNew.
  friend uint qHash(const SExpression& node, uint seed) noexcept;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

uint qHash(const SExpression& node, uint seed = 0) noexcept;
uint qHash(const std::unique_ptr<SExpression>& ptr, uint seed = 0) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
