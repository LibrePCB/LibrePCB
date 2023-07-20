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

#ifndef LIBREPCB_CORE_COMPONENTASSEMBLYOPTION_H
#define LIBREPCB_CORE_COMPONENTASSEMBLYOPTION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../library/dev/part.h"
#include "../../serialization/serializableobjectlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ComponentAssemblyOption
 ******************************************************************************/

/**
 * @brief The ComponentAssemblyOption class
 */
class ComponentAssemblyOption final {
public:
  // Signals
  enum class Event {
    DeviceChanged,
    AttributesChanged,
    PartsEdited,
  };
  Signal<ComponentAssemblyOption, Event> onEdited;
  typedef Slot<ComponentAssemblyOption, Event> OnEditedSlot;

  // Constructors / Destructor
  ComponentAssemblyOption() = delete;
  ComponentAssemblyOption(const ComponentAssemblyOption& other) noexcept;
  explicit ComponentAssemblyOption(const SExpression& node);
  ComponentAssemblyOption(const Uuid& device, const AttributeList& attributes,
                          const PartList& parts);
  ~ComponentAssemblyOption() noexcept;

  // Getters
  const Uuid& getDevice() const noexcept { return mDevice; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }
  PartList& getParts() noexcept { return mParts; }
  const PartList& getParts() const noexcept { return mParts; }

  // Setters
  void setDevice(const Uuid& value) noexcept;
  void setAttributes(const AttributeList& value) noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  ComponentAssemblyOption& operator=(const ComponentAssemblyOption& rhs) =
      delete;
  bool operator==(const ComponentAssemblyOption& rhs) const noexcept;
  bool operator!=(const ComponentAssemblyOption& rhs) const noexcept {
    return !(*this == rhs);
  }

private:  // Methods
  void partListEdited(const PartList& list, int index,
                      const std::shared_ptr<const Part>& obj,
                      PartList::Event event) noexcept;

private:  // Data
  /// Compatible device.
  Uuid mDevice;

  /// Default attributes to copy when adding new parts
  AttributeList mAttributes;

  /// Parts available for assembly
  PartList mParts;

  // Slots
  PartList::OnEditedSlot mOnPartsEditedSlot;
};

/*******************************************************************************
 *  Class ComponentAssemblyOptionList
 ******************************************************************************/

struct ComponentAssemblyOptionListNameProvider {
  static constexpr const char* tagname = "device";
};
using ComponentAssemblyOptionList =
    SerializableObjectList<ComponentAssemblyOption,
                           ComponentAssemblyOptionListNameProvider,
                           ComponentAssemblyOption::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
