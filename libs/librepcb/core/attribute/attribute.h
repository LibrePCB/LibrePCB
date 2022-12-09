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

#ifndef LIBREPCB_CORE_ATTRIBUTE_H
#define LIBREPCB_CORE_ATTRIBUTE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "attributekey.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeType;
class AttributeUnit;

/*******************************************************************************
 *  Class Attribute
 ******************************************************************************/

/**
 * @brief The Attribute class
 */
class Attribute final {
  Q_DECLARE_TR_FUNCTIONS(Attribute)

public:
  // Signals
  enum class Event {
    KeyChanged,
    TypeValueUnitChanged,
  };
  Signal<Attribute, Event> onEdited;
  typedef Slot<Attribute, Event> OnEditedSlot;

  // Constructors / Destructor
  Attribute() = delete;
  Attribute(const Attribute& other) noexcept;
  explicit Attribute(const SExpression& node);
  Attribute(const AttributeKey& key, const AttributeType& type,
            const QString& value, const AttributeUnit* unit);
  ~Attribute() noexcept;

  // Getters
  const AttributeKey& getKey() const noexcept { return mKey; }
  const QString& getName() const noexcept {
    return *mKey;
  }  // required for SerializableObjectList
  const AttributeType& getType() const noexcept { return *mType; }
  const AttributeUnit* getUnit() const noexcept { return mUnit; }
  const QString& getValue() const noexcept { return mValue; }
  QString getValueTr(bool showUnit) const noexcept;

  // Setters
  bool setKey(const AttributeKey& key) noexcept;
  bool setTypeValueUnit(const AttributeType& type, const QString& value,
                        const AttributeUnit* unit);

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Attribute& rhs) const noexcept;
  bool operator!=(const Attribute& rhs) const noexcept {
    return !(*this == rhs);
  }
  Attribute& operator=(const Attribute& rhs) = delete;

private:  // Methods
  bool checkAttributesValidity() const noexcept;

private:  // Data
  AttributeKey mKey;
  const AttributeType* mType;
  QString mValue;
  const AttributeUnit* mUnit;
};

/*******************************************************************************
 *  Class AttributeList
 ******************************************************************************/

struct AttributeListNameProvider {
  static constexpr const char* tagname = "attribute";
};
using AttributeList =
    SerializableObjectList<Attribute, AttributeListNameProvider,
                           Attribute::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
