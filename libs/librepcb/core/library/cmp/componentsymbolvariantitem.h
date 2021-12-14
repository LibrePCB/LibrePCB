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

#ifndef LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANTITEM_H
#define LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANTITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "componentpinsignalmap.h"
#include "componentsymbolvariantitemsuffix.h"

#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/units/angle.h>
#include <librepcb/common/units/point.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class ComponentSymbolVariantItem
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantItem class represents one symbol of a
 * component symbol variant
 *
 * Following information is considered as the "interface" of a symbol variant
 * item and must therefore never be changed:
 *  - UUID
 *  - Symbol UUID
 *  - Pin-signal-mapping
 */
class ComponentSymbolVariantItem final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(ComponentSymbolVariantItem)

public:
  // Signals
  enum class Event {
    UuidChanged,
    SymbolUuidChanged,
    SymbolPositionChanged,
    SymbolRotationChanged,
    IsRequiredChanged,
    SuffixChanged,
    PinSignalMapEdited,
  };
  Signal<ComponentSymbolVariantItem, Event> onEdited;
  typedef Slot<ComponentSymbolVariantItem, Event> OnEditedSlot;

  // Constructors / Destructor
  ComponentSymbolVariantItem() = delete;
  ComponentSymbolVariantItem(const ComponentSymbolVariantItem& other) noexcept;
  ComponentSymbolVariantItem(
      const Uuid& uuid, const Uuid& symbolUuid, const Point& symbolPos,
      const Angle& symbolRotation, bool isRequired,
      const ComponentSymbolVariantItemSuffix& suffix) noexcept;
  ComponentSymbolVariantItem(const SExpression& node,
                             const Version& fileFormat);
  ~ComponentSymbolVariantItem() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Uuid& getSymbolUuid() const noexcept { return mSymbolUuid; }
  const Point& getSymbolPosition() const noexcept { return mSymbolPos; }
  const Angle& getSymbolRotation() const noexcept { return mSymbolRot; }
  bool isRequired() const noexcept { return mIsRequired; }
  const ComponentSymbolVariantItemSuffix& getSuffix() const noexcept {
    return mSuffix;
  }

  // Setters: Attributes
  bool setSymbolUuid(const Uuid& uuid) noexcept;
  bool setSymbolPosition(const Point& pos) noexcept;
  bool setSymbolRotation(const Angle& rot) noexcept;
  bool setIsRequired(bool required) noexcept;
  bool setSuffix(const ComponentSymbolVariantItemSuffix& suffix) noexcept;

  // Pin-Signal-Map Methods
  ComponentPinSignalMap& getPinSignalMap() noexcept { return mPinSignalMap; }
  const ComponentPinSignalMap& getPinSignalMap() const noexcept {
    return mPinSignalMap;
  }

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const ComponentSymbolVariantItem& rhs) const noexcept;
  bool operator!=(const ComponentSymbolVariantItem& rhs) const noexcept {
    return !(*this == rhs);
  }
  ComponentSymbolVariantItem& operator=(
      const ComponentSymbolVariantItem& rhs) noexcept;

private:  // Methods
  void pinSignalMapEdited(
      const ComponentPinSignalMap& map, int index,
      const std::shared_ptr<const ComponentPinSignalMapItem>& item,
      ComponentPinSignalMap::Event event) noexcept;

private:  // Data
  Uuid mUuid;
  Uuid mSymbolUuid;
  Point mSymbolPos;
  Angle mSymbolRot;
  bool mIsRequired;
  ComponentSymbolVariantItemSuffix mSuffix;
  ComponentPinSignalMap mPinSignalMap;

  // Slots
  ComponentPinSignalMap::OnEditedSlot mOnPinSignalMapEditedSlot;
};

/*******************************************************************************
 *  Class ComponentSymbolVariantItemList
 ******************************************************************************/

struct ComponentSymbolVariantItemListNameProvider {
  static constexpr const char* tagname = "gate";
};
using ComponentSymbolVariantItemList =
    SerializableObjectList<ComponentSymbolVariantItem,
                           ComponentSymbolVariantItemListNameProvider,
                           ComponentSymbolVariantItem::Event>;

/*******************************************************************************
 *  Class ComponentSymbolVariantItemListHelpers
 ******************************************************************************/

class ComponentSymbolVariantItemListHelpers {
public:
  ComponentSymbolVariantItemListHelpers() = delete;  // disable instantiation

  static QSet<Uuid> getAllSymbolUuids(
      const ComponentSymbolVariantItemList& list) noexcept {
    QSet<Uuid> set;
    for (const auto& item : list) {
      set.insert(item.getSymbolUuid());
    }
    return set;
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif
