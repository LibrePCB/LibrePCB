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

#ifndef LIBREPCB_CORE_PART_H
#define LIBREPCB_CORE_PART_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attribute.h"
#include "../../serialization/serializableobjectlist.h"
#include "../../types/simplestring.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Part
 ******************************************************************************/

/**
 * @brief The Part class
 */
class Part final {
  Q_DECLARE_TR_FUNCTIONS(Part)

public:
  // Signals
  enum class Event {
    MpnChanged,
    ManufacturerChanged,
    AttributesEdited,
  };
  Signal<Part, Event> onEdited;
  typedef Slot<Part, Event> OnEditedSlot;

  // Constructors / Destructor
  Part() = delete;
  Part(const Part& other) noexcept;
  explicit Part(const SExpression& node);
  Part(const SimpleString& mpn, const SimpleString& manufacturer,
       const AttributeList& attributes) noexcept;
  ~Part() noexcept;

  // Getters
  bool isEmpty() const noexcept;
  const SimpleString& getMpn() const noexcept { return mMpn; }
  const SimpleString& getManufacturer() const noexcept { return mManufacturer; }
  AttributeList& getAttributes() noexcept { return mAttributes; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }
  QStringList getAttributeValuesTr() const noexcept;
  QStringList getAttributeKeyValuesTr() const noexcept;

  // Setters
  void setMpn(const SimpleString& value) noexcept;
  void setManufacturer(const SimpleString& value) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Part& rhs) const noexcept;
  bool operator!=(const Part& rhs) const noexcept { return !(*this == rhs); }
  Part& operator=(const Part& rhs) noexcept;

private:  // Methods
  void attributeListEdited(const AttributeList& list, int index,
                           const std::shared_ptr<const Attribute>& attribute,
                           AttributeList::Event event) noexcept;

private:  // Data
  SimpleString mMpn;
  SimpleString mManufacturer;
  AttributeList mAttributes;

  // Slots
  AttributeList::OnEditedSlot mOnAttributesEditedSlot;
};

/*******************************************************************************
 *  Class PartList
 ******************************************************************************/

struct PartListNameProvider {
  static constexpr const char* tagname = "part";
};
using PartList =
    SerializableObjectList<Part, PartListNameProvider, Part::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(std::shared_ptr<librepcb::Part>)

#endif
