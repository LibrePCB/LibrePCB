/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/uuid.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include "componentpinsignalmap.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class ComponentSymbolVariantItem
 ****************************************************************************************/

/**
 * @brief The ComponentSymbolVariantItem class represents one symbol of a component symbol
 *        variant
 *
 * Following information is considered as the "interface" of a symbol variant item and
 * must therefore never be changed:
 *  - UUID
 *  - Symbol UUID
 *  - Pin-signal-mapping
 */
class ComponentSymbolVariantItem final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentSymbolVariantItem)

    public:

        // Constructors / Destructor
        ComponentSymbolVariantItem() = delete;
        ComponentSymbolVariantItem(const ComponentSymbolVariantItem& other) noexcept;
        ComponentSymbolVariantItem(const Uuid& uuid, const Uuid& symbolUuid,
                                   bool isRequired, const QString& suffix) noexcept;
        explicit ComponentSymbolVariantItem(const SExpression& node);
        ~ComponentSymbolVariantItem() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Uuid& getSymbolUuid() const noexcept {return mSymbolUuid;}
        const Point& getSymbolPosition() const noexcept {return mSymbolPos;}
        const Angle& getSymbolRotation() const noexcept {return mSymbolRot;}
        bool isRequired() const noexcept {return mIsRequired;}
        const QString& getSuffix() const noexcept {return mSuffix;}

        // Setters: Attributes
        void setSymbolUuid(const Uuid& uuid) noexcept {mSymbolUuid = uuid;}
        void setSymbolPosition(const Point& pos) noexcept {mSymbolPos = pos;}
        void setSymbolRotation(const Angle& rot) noexcept {mSymbolRot = rot;}
        void setIsRequired(bool required) noexcept {mIsRequired = required;}
        void setSuffix(const QString& suffix) noexcept {mSuffix = suffix;}

        // Pin-Signal-Map Methods
        ComponentPinSignalMap& getPinSignalMap() noexcept {return mPinSignalMap;}
        const ComponentPinSignalMap& getPinSignalMap()const  noexcept {return mPinSignalMap;}

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const ComponentSymbolVariantItem& rhs) const noexcept;
        bool operator!=(const ComponentSymbolVariantItem& rhs) const noexcept {return !(*this == rhs);}
        ComponentSymbolVariantItem& operator=(const ComponentSymbolVariantItem& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        Uuid mSymbolUuid;
        Point mSymbolPos;
        Angle mSymbolRot;
        bool mIsRequired;
        QString mSuffix;
        ComponentPinSignalMap mPinSignalMap;
};

/*****************************************************************************************
 *  Class ComponentSymbolVariantItemList
 ****************************************************************************************/

struct ComponentSymbolVariantItemListNameProvider {static constexpr const char* tagname = "item";};
using ComponentSymbolVariantItemList = SerializableObjectList<ComponentSymbolVariantItem, ComponentSymbolVariantItemListNameProvider>;
using CmdComponentSymbolVariantItemInsert = CmdListElementInsert<ComponentSymbolVariantItem, ComponentSymbolVariantItemListNameProvider>;
using CmdComponentSymbolVariantItemRemove = CmdListElementRemove<ComponentSymbolVariantItem, ComponentSymbolVariantItemListNameProvider>;
using CmdComponentSymbolVariantItemsSwap = CmdListElementsSwap<ComponentSymbolVariantItem, ComponentSymbolVariantItemListNameProvider>;

/*****************************************************************************************
 *  Class ComponentSymbolVariantItemListHelpers
 ****************************************************************************************/

class ComponentSymbolVariantItemListHelpers
{
    public:
        ComponentSymbolVariantItemListHelpers() = delete; // disable instantiation

        static QSet<Uuid> getAllSymbolUuids(const ComponentSymbolVariantItemList& list) noexcept {
            QSet<Uuid> set;
            for (const auto& item : list) {
                set.insert(item.getSymbolUuid());
            }
            return set;
        }
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANTITEM_H
