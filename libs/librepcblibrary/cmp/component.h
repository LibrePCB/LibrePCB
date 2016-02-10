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

#ifndef LIBREPCB_LIBRARY_COMPONENT_H
#define LIBREPCB_LIBRARY_COMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../libraryelement.h"
#include "componentsignal.h"
#include "componentsymbolvariant.h"
#include "../libraryelementattribute.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Component
 ****************************************************************************************/

/**
 * @brief The Component class
 */
class Component final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Component(const Uuid& uuid, const Version& version, const QString& author,
                           const QString& name_en_US, const QString& description_en_US,
                           const QString& keywords_en_US) throw (Exception);
        explicit Component(const FilePath& elementDirectory, bool readOnly) throw (Exception);
        ~Component() noexcept;

        // General
        bool isSchematicOnly() const noexcept {return mSchematicOnly;}
        void setIsSchematicOnly(bool schematicOnly) noexcept {mSchematicOnly = schematicOnly;}

        // Attribute Methods
        const QList<LibraryElementAttribute*>& getAttributes() noexcept {return mAttributes;}
        int getAttributeCount() const noexcept {return mAttributes.count();}
        LibraryElementAttribute* getAttribute(int index) noexcept {return mAttributes.value(index);}
        const LibraryElementAttribute* getAttribute(int index) const noexcept {return mAttributes.value(index);}
        LibraryElementAttribute* getAttributeByKey(const QString& key) noexcept;
        const LibraryElementAttribute* getAttributeByKey(const QString& key) const noexcept;
        void addAttribute(LibraryElementAttribute& attr) noexcept;
        void removeAttribute(LibraryElementAttribute& attr) noexcept;

        // Default Value Methods
        const QMap<QString, QString>& getDefaultValues() const noexcept {return mDefaultValues;}
        QString getDefaultValue(const QStringList& localeOrder) const throw (Exception);
        void addDefaultValue(const QString& locale, const QString& value) noexcept;
        void removeDefaultValue(const QString& locale) noexcept;

        // Prefix Methods
        const QMap<QString, QString>& getPrefixes() const noexcept {return mPrefixes;}
        QString getPrefix(const QStringList& normOrder) const noexcept;
        QString getDefaultPrefix() const noexcept;
        void addPrefix(const QString& norm, const QString& prefix) noexcept;

        // Signal Methods
        const QList<ComponentSignal*>& getSignals() noexcept {return mSignals;}
        int getSignalCount() const noexcept {return mSignals.count();}
        ComponentSignal* getSignal(int index) noexcept {return mSignals.value(index);}
        const ComponentSignal* getSignal(int index) const noexcept {return mSignals.value(index);}
        ComponentSignal* getSignalByUuid(const Uuid& uuid) noexcept;
        const ComponentSignal* getSignalByUuid(const Uuid& uuid) const noexcept;
        ComponentSignal* getSignalOfPin(const Uuid& symbVar, const Uuid& item, const Uuid& pin) noexcept;
        const ComponentSignal* getSignalOfPin(const Uuid& symbVar, const Uuid& item, const Uuid& pin) const noexcept;
        void addSignal(ComponentSignal& signal) noexcept;
        void removeSignal(ComponentSignal& signal) noexcept;

        // Symbol Variant Methods
        const QList<ComponentSymbolVariant*>& getSymbolVariants() noexcept {return mSymbolVariants;}
        int getSymbolVariantCount() const noexcept {return mSymbolVariants.count();}
        ComponentSymbolVariant* getSymbolVariant(int index) noexcept {return mSymbolVariants.value(index);}
        const ComponentSymbolVariant* getSymbolVariant(int index) const noexcept {return mSymbolVariants.value(index);}
        ComponentSymbolVariant* getSymbolVariantByUuid(const Uuid& uuid) noexcept;
        const ComponentSymbolVariant* getSymbolVariantByUuid(const Uuid& uuid) const noexcept;
        const Uuid& getDefaultSymbolVariantUuid() const noexcept {return mDefaultSymbolVariantUuid;}
        ComponentSymbolVariant* getDefaultSymbolVariant() noexcept;
        const ComponentSymbolVariant* getDefaultSymbolVariant() const noexcept;
        void addSymbolVariant(ComponentSymbolVariant& symbolVariant) noexcept;
        void removeSymbolVariant(ComponentSymbolVariant& symbolVariant) noexcept;

        // Symbol Variant Item Methods
        ComponentSymbolVariantItem* getSymbVarItem(const Uuid& symbVar, const Uuid& item) noexcept;
        const ComponentSymbolVariantItem* getSymbVarItem(const Uuid& symbVar, const Uuid& item) const noexcept;


    private:

        // make some methods inaccessible...
        Component() = delete;
        Component(const Component& other) = delete;
        Component& operator=(const Component& rhs) = delete;


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Conponent Attributes
        bool mSchematicOnly; ///< if true, this component is schematic-only (no package)
        QList<LibraryElementAttribute*> mAttributes; ///< all attributes in a specific order
        QMap<QString, QString> mDefaultValues; ///< key: locale (like "en_US"), value: default value
        QMap<QString, QString> mPrefixes; ///< key: norm, value: prefix
        QList<ComponentSignal*> mSignals; ///< empty if the component has no signals
        QList<ComponentSymbolVariant*> mSymbolVariants; ///< minimum one entry
        Uuid mDefaultSymbolVariantUuid; ///< must be an existing key of #mSymbolVariants
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENT_H
