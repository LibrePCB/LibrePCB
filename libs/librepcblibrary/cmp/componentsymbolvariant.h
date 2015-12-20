/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBRARY_COMPONENTSYMBOLVARIANT_H
#define LIBRARY_COMPONENTSYMBOLVARIANT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include "componentsymbolvariantitem.h"

/*****************************************************************************************
 *  Class ComponentSymbolVariant
 ****************************************************************************************/

namespace library {

/**
 * @brief The ComponentSymbolVariant class
 */
class ComponentSymbolVariant final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentSymbolVariant)

    public:

        // Constructors / Destructor
        explicit ComponentSymbolVariant(const QUuid& uuid = QUuid::createUuid(),
                                        const QString& norm = QString(),
                                        bool isDefault = false) noexcept;
        explicit ComponentSymbolVariant(const XmlDomElement& domElement) throw (Exception);
        ~ComponentSymbolVariant() noexcept;

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getNorm() const noexcept {return mNorm;}
        bool isDefault() const noexcept {return mIsDefault;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // Getters: Symbol Items
        const QList<const ComponentSymbolVariantItem*>& getItems() const noexcept {return mSymbolItems;}
        const ComponentSymbolVariantItem* getItemByUuid(const QUuid& uuid) const noexcept;
        const ComponentSymbolVariantItem* getNextItem(const ComponentSymbolVariantItem* item) const noexcept;

        // Setters
        void setNorm(const QString& norm) noexcept;
        void setIsDefault(bool isDefault) noexcept;
        void setName(const QString& locale, const QString& name) noexcept;
        void setDescription(const QString& locale, const QString& desc) noexcept;

        // General Methods
        void clearItems() noexcept;
        void addItem(const ComponentSymbolVariantItem& item) noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        ComponentSymbolVariant(const ComponentSymbolVariant& other);
        ComponentSymbolVariant& operator=(const ComponentSymbolVariant& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Symbol Variant Attributes
        QUuid mUuid;
        QString mNorm;
        bool mIsDefault;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
        QList<const ComponentSymbolVariantItem*> mSymbolItems; ///< minimum one item
};

} // namespace library

#endif // LIBRARY_COMPONENTSYMBOLVARIANT_H
