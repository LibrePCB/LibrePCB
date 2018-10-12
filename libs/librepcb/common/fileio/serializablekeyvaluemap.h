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

#ifndef LIBREPCB_SERIALIZABLEKEYVALUEMAP_H
#define LIBREPCB_SERIALIZABLEKEYVALUEMAP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../elementname.h"
#include "serializableobject.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SerializableKeyValueMap
 ******************************************************************************/

/**
 * @brief The SerializableKeyValueMap class provides an easy way to serialize
 * and deserialize ordered key value pairs
 *
 * @note This map guarantees to always contain a valid default value. A default
 * value has an empty string as key and will be used as fallback for #value().
 *
 * @warning When modifying this class, make sure that it still guarantees to
 * always contain a valid default value! So, don't add a default constructor, a
 *          `clear()` method or similar!
 */
template <typename T>
class SerializableKeyValueMap final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(SerializableKeyValueMap)

public:
  // Constructors / Destructor
  SerializableKeyValueMap() = delete;
  SerializableKeyValueMap(const SerializableKeyValueMap<T>& other) noexcept
    : mValues(other.mValues) {}
  explicit SerializableKeyValueMap(
      const typename T::ValueType& defaultValue) noexcept {
    mValues.insert("", defaultValue);
  }
  explicit SerializableKeyValueMap(const SExpression& node) {
    foreach (const SExpression& child, node.getChildren(T::tagname)) {
      QString     key;
      SExpression value;
      if (child.getChildren().count() > 1) {
        key   = child.getValueByPath<QString>(T::keyname);
        value = child.getChildByIndex(1);
      } else {
        key   = QString("");
        value = child.getChildByIndex(0);
      }
      if (mValues.contains(key)) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString(tr("Key \"%1\" defined multiple times.")).arg(key));
      }
      mValues.insert(key, deserializeFromSExpression<typename T::ValueType>(
                              value, false));  // can throw
    }
    if (!mValues.contains(QString(""))) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString(tr("No default %1 defined.")).arg(T::tagname));
    }
  }
  ~SerializableKeyValueMap() noexcept {}

  // Getters
  QStringList                  keys() const noexcept { return mValues.keys(); }
  const typename T::ValueType& getDefaultValue() const noexcept {
    auto i = mValues.find(QString(""));
    // there must always be a default value!!!
    Q_ASSERT((i != mValues.end()) && (i.key() == QString("")));
    return i.value();
  }
  bool contains(const QString& key) const noexcept {
    return mValues.contains(key);
  }
  tl::optional<typename T::ValueType> tryGet(const QString& key) const
      noexcept {
    auto i = mValues.find(key);
    if ((i != mValues.end()) && (i.key() == key)) {
      return i.value();
    } else {
      return tl::nullopt;
    }
  }
  const typename T::ValueType& value(const QStringList& keyOrder,
                                     QString*           usedKey = nullptr) const
      noexcept {
    // search in the specified key order
    foreach (const QString& key, keyOrder) {
      auto i = mValues.find(key);
      if ((i != mValues.end()) && (i.key() == key)) {
        if (usedKey) *usedKey = key;
        return i.value();
      }
    }
    // use default value (empty key) as fallback
    if (usedKey) *usedKey = QString("");
    return getDefaultValue();
  }

  // General Methods

  void setDefaultValue(const typename T::ValueType& value) noexcept {
    insert(QString(""), value);
  }

  void insert(const QString& key, const typename T::ValueType& value) noexcept {
    mValues.insert(key, value);
  }

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override {
    for (auto i = mValues.constBegin(); i != mValues.constEnd(); ++i) {
      SExpression& child = root.appendList(T::tagname, true);
      if (!i.key().isEmpty()) {
        child.appendChild(T::keyname, i.key(), false);
      }
      child.appendChild(i.value());
    }
  }

  // Operator Overloadings
  SerializableKeyValueMap<T>& operator=(
      const SerializableKeyValueMap<T>& rhs) noexcept {
    mValues = rhs.mValues;
    return *this;
  }
  bool operator==(const SerializableKeyValueMap<T>& rhs) const noexcept {
    return mValues == rhs.mValues;
  }
  bool operator!=(const SerializableKeyValueMap<T>& rhs) const noexcept {
    return mValues != rhs.mValues;
  }

private:  // Data
  QMap<QString, typename T::ValueType> mValues;
};

/*******************************************************************************
 *  Class LocalizedNameMap
 ******************************************************************************/

struct LocalizedNameMapPolicy {
  typedef ElementName          ValueType;
  static constexpr const char* tagname = "name";
  static constexpr const char* keyname = "locale";
};
using LocalizedNameMap = SerializableKeyValueMap<LocalizedNameMapPolicy>;

/*******************************************************************************
 *  Class LocalizedDescriptionMap
 ******************************************************************************/

struct LocalizedDescriptionMapPolicy {
  typedef QString              ValueType;
  static constexpr const char* tagname = "description";
  static constexpr const char* keyname = "locale";
};
using LocalizedDescriptionMap =
    SerializableKeyValueMap<LocalizedDescriptionMapPolicy>;

/*******************************************************************************
 *  Class LocalizedKeywordsMap
 ******************************************************************************/

struct LocalizedKeywordsMapPolicy {
  typedef QString              ValueType;
  static constexpr const char* tagname = "keywords";
  static constexpr const char* keyname = "locale";
};
using LocalizedKeywordsMap =
    SerializableKeyValueMap<LocalizedKeywordsMapPolicy>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SERIALIZABLEKEYVALUEMAP_H
