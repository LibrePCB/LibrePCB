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

#ifndef LIBREPCB_CORE_COMPONENTSYMBOLVARIANT_H
#define LIBREPCB_CORE_COMPONENTSYMBOLVARIANT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializablekeyvaluemap.h"
#include "../../serialization/serializableobjectlist.h"
#include "componentsymbolvariantitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ComponentSymbolVariant
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariant class represents a symbol variant of a
 * component
 *
 * Following information is considered as the "interface" of a symbol variant
 * and must therefore never be changed:
 *  - UUID
 *  - Symbol items (neither adding nor removing items is allowed)
 *    - UUID
 *    - Symbol UUID
 *    - Pin-signal-mapping
 */
class ComponentSymbolVariant final : public SerializableObject {
public:
  // Signals
  enum class Event {
    UuidChanged,
    NormChanged,
    NamesChanged,
    DescriptionsChanged,
    SymbolItemsEdited,
  };
  Signal<ComponentSymbolVariant, Event> onEdited;
  typedef Slot<ComponentSymbolVariant, Event> OnEditedSlot;

  // Constructors / Destructor
  ComponentSymbolVariant() = delete;
  ComponentSymbolVariant(const ComponentSymbolVariant& other) noexcept;
  ComponentSymbolVariant(const Uuid& uuid, const QString& norm,
                         const ElementName& name_en_US,
                         const QString& desc_en_US) noexcept;
  ComponentSymbolVariant(const SExpression& node, const Version& fileFormat);
  ~ComponentSymbolVariant() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const QString& getNorm() const noexcept { return mNorm; }
  const ElementName& getName() const noexcept {
    return mNames.getDefaultValue();  // Used for SerializableObjectList
  }
  const LocalizedNameMap& getNames() const noexcept { return mNames; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }

  // Setters
  bool setNorm(const QString& norm) noexcept;
  bool setName(const QString& locale, const ElementName& name) noexcept;
  bool setDescription(const QString& locale, const QString& desc) noexcept;
  bool setNames(const LocalizedNameMap& names) noexcept;
  bool setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept;

  // Symbol Item Methods
  ComponentSymbolVariantItemList& getSymbolItems() noexcept {
    return mSymbolItems;
  }
  const ComponentSymbolVariantItemList& getSymbolItems() const noexcept {
    return mSymbolItems;
  }
  QSet<Uuid> getAllSymbolUuids() const noexcept {
    return ComponentSymbolVariantItemListHelpers::getAllSymbolUuids(
        mSymbolItems);
  }

  // General Methods

  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const ComponentSymbolVariant& rhs) const noexcept;
  bool operator!=(const ComponentSymbolVariant& rhs) const noexcept {
    return !(*this == rhs);
  }
  ComponentSymbolVariant& operator=(const ComponentSymbolVariant& rhs) noexcept;

private:  // Methods
  void itemsEdited(
      const ComponentSymbolVariantItemList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariantItem>& item,
      ComponentSymbolVariantItemList::Event event) noexcept;

private:  // Data
  Uuid mUuid;
  QString mNorm;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  ComponentSymbolVariantItemList mSymbolItems;

  // Slots
  ComponentSymbolVariantItemList::OnEditedSlot mOnItemsEditedSlot;
};

/*******************************************************************************
 *  Class ComponentSymbolVariantList
 ******************************************************************************/

struct ComponentSymbolVariantListNameProvider {
  static constexpr const char* tagname = "variant";
};
using ComponentSymbolVariantList =
    SerializableObjectList<ComponentSymbolVariant,
                           ComponentSymbolVariantListNameProvider,
                           ComponentSymbolVariant::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
