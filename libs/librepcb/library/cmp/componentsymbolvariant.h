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

#ifndef LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANT_H
#define LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/fileio/serializablekeyvaluemap.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include "componentsymbolvariantitem.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class ComponentSymbolVariant
 ****************************************************************************************/

/**
 * @brief The ComponentSymbolVariant class represents a symbol variant of a component
 *
 * Following information is considered as the "interface" of a symbol variant and must
 * therefore never be changed:
 *  - UUID
 *  - Symbol items (neither adding nor removing items is allowed)
 *    - UUID
 *    - Symbol UUID
 *    - Pin-signal-mapping
 */
class ComponentSymbolVariant final : public QObject, public SerializableObject,
                                     private ComponentSymbolVariantItemList::IF_Observer
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        ComponentSymbolVariant() = delete;
        ComponentSymbolVariant(const ComponentSymbolVariant& other) noexcept;
        ComponentSymbolVariant(const Uuid& uuid, const QString& norm,
                               const QString& name_en_US, const QString& desc_en_US) noexcept;
        explicit ComponentSymbolVariant(const DomElement& domElement);
        ~ComponentSymbolVariant() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getNorm() const noexcept {return mNorm;}
        const LocalizedNameMap& getNames() const noexcept {return mNames;}
        const LocalizedDescriptionMap& getDescriptions() const noexcept {return mDescriptions;}

        // Setters
        void setNorm(const QString& norm) noexcept;
        void setName(const QString& locale, const QString& name) noexcept;
        void setDescription(const QString& locale, const QString& desc) noexcept;
        void setNames(const LocalizedNameMap& names) noexcept;
        void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept;

        // Symbol Item Methods
        ComponentSymbolVariantItemList& getSymbolItems() noexcept {return mSymbolItems;}
        const ComponentSymbolVariantItemList& getSymbolItems() const noexcept {return mSymbolItems;}
        QSet<Uuid> getAllSymbolUuids() const noexcept {return ComponentSymbolVariantItemListHelpers::getAllSymbolUuids(mSymbolItems);}

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const override;

        // Operator Overloadings
        bool operator==(const ComponentSymbolVariant& rhs) const noexcept;
        bool operator!=(const ComponentSymbolVariant& rhs) const noexcept {return !(*this == rhs);}
        ComponentSymbolVariant& operator=(const ComponentSymbolVariant& rhs) noexcept;


    signals:
        void edited();


    private: // Methods
        void listObjectAdded(const ComponentSymbolVariantItemList& list, int newIndex,
                             const std::shared_ptr<ComponentSymbolVariantItem>& ptr) noexcept override;
        void listObjectRemoved(const ComponentSymbolVariantItemList& list, int oldIndex,
                               const std::shared_ptr<ComponentSymbolVariantItem>& ptr) noexcept override;
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        QString mNorm;
        LocalizedNameMap mNames;
        LocalizedDescriptionMap mDescriptions;
        ComponentSymbolVariantItemList mSymbolItems;
};

/*****************************************************************************************
 *  Class ComponentSymbolVariantList
 ****************************************************************************************/

struct ComponentSymbolVariantListNameProvider {static constexpr const char* tagname = "symbol_variant";};
using ComponentSymbolVariantList = SerializableObjectList<ComponentSymbolVariant, ComponentSymbolVariantListNameProvider>;
using CmdComponentSymbolVariantInsert = CmdListElementInsert<ComponentSymbolVariant, ComponentSymbolVariantListNameProvider>;
using CmdComponentSymbolVariantRemove = CmdListElementRemove<ComponentSymbolVariant, ComponentSymbolVariantListNameProvider>;
using CmdComponentSymbolVariantsSwap = CmdListElementsSwap<ComponentSymbolVariant, ComponentSymbolVariantListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANT_H
