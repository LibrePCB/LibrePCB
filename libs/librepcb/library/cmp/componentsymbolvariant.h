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
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/serializablekeyvaluemap.h>
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
 * @brief The ComponentSymbolVariant class
 */
class ComponentSymbolVariant final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentSymbolVariant)

    public:

        // Constructors / Destructor
        explicit ComponentSymbolVariant(const Uuid& uuid, const QString& norm,
                                        const QString& name_en_US,
                                        const QString& desc_en_US) noexcept;
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

        // Symbol Item Methods
        const QList<ComponentSymbolVariantItem*>& getItems() noexcept {return mSymbolItems;}
        int getItemCount() const noexcept {return mSymbolItems.count();}
        ComponentSymbolVariantItem* getItem(int index) noexcept {return mSymbolItems.value(index);}
        const ComponentSymbolVariantItem* getItem(int index) const noexcept {return mSymbolItems.value(index);}
        ComponentSymbolVariantItem* getItemByUuid(const Uuid& uuid) noexcept;
        const ComponentSymbolVariantItem* getItemByUuid(const Uuid& uuid) const noexcept;
        QSet<Uuid> getAllItemSymbolUuids() const noexcept;
        void addItem(ComponentSymbolVariantItem& item) noexcept;
        void removeItem(ComponentSymbolVariantItem& item) noexcept;

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const override;


    private:

        // make some methods inaccessible...
        ComponentSymbolVariant() = delete;
        ComponentSymbolVariant(const ComponentSymbolVariant& other) = delete;
        ComponentSymbolVariant& operator=(const ComponentSymbolVariant& rhs) = delete;

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Symbol Variant Attributes
        Uuid mUuid;
        QString mNorm;
        LocalizedNameMap mNames;
        LocalizedDescriptionMap mDescriptions;
        QList<ComponentSymbolVariantItem*> mSymbolItems; ///< minimum one item
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENTSYMBOLVARIANT_H
