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

#ifndef LIBRARY_COMPONENT_H
#define LIBRARY_COMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"
#include "componentsignal.h"
#include "componentsymbolvariant.h"
#include "../libraryelementattribute.h"

/*****************************************************************************************
 *  Class Component
 ****************************************************************************************/

namespace library {

/**
 * @brief The Component class
 */
class Component final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Component(const Uuid& uuid = Uuid::createRandom(),
                           const Version& version = Version(),
                           const QString& author = QString(),
                           const QString& name_en_US = QString(),
                           const QString& description_en_US = QString(),
                           const QString& keywords_en_US = QString()) throw (Exception);
        explicit Component(const FilePath& elementDirectory) throw (Exception);
        ~Component() noexcept;

        // General
        bool isSchematicOnly() const noexcept {return mSchematicOnly;}
        void setIsSchematicOnly(bool schematicOnly) noexcept {mSchematicOnly = schematicOnly;}

        // Attributes
        const QList<LibraryElementAttribute*>& getAttributes() const noexcept;
        const LibraryElementAttribute* getAttributeByKey(const QString& key) const noexcept;

        // Default Values
        const QMap<QString, QString>& getDefaultValues() const noexcept;
        QString getDefaultValue(const QStringList& localeOrder) const noexcept;
        void clearDefaultValues() noexcept;
        void addDefaultValue(const QString& locale, const QString& value) noexcept;

        // Prefixes
        const QMap<QString, QString>& getPrefixes() const noexcept;
        QString getPrefix(const QStringList& normOrder) const noexcept;
        const QString& getDefaultPrefixNorm() const noexcept;
        QString getDefaultPrefix() const noexcept;
        void clearPrefixes() noexcept;
        void addPrefix(const QString& norm, const QString& prefix, bool isDefault) noexcept;

        // Signals
        const QList<const ComponentSignal*>& getSignals() const noexcept;
        const ComponentSignal* getSignalByUuid(const Uuid& uuid) const noexcept;
        const ComponentSignal* getSignalOfPin(const Uuid& symbVarUuid, const Uuid& itemUuid,
                                              const Uuid& pinUuid) const noexcept;
        void clearSignals() noexcept;
        void addSignal(const ComponentSignal& signal) noexcept;

        // Symbol Variants
        const QList<const ComponentSymbolVariant*>& getSymbolVariants() const noexcept;
        const ComponentSymbolVariant* getSymbolVariantByUuid(const Uuid& uuid) const noexcept;
        const Uuid& getDefaultSymbolVariantUuid() const noexcept;
        const ComponentSymbolVariant* getDefaultSymbolVariant() const noexcept;
        void clearSymbolVariants() noexcept;
        void addSymbolVariant(const ComponentSymbolVariant& symbolVariant) noexcept;

        // Symbol Variant Items
        const ComponentSymbolVariantItem* getSymbVarItem(const Uuid& symbVarUuid,
                                                         const Uuid& itemUuid) const noexcept;

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


        // Generic Conponent Attributes
        bool mSchematicOnly; ///< if true, this component is schematic-only (no package)
        QList<LibraryElementAttribute*> mAttributes; ///< all attributes in a specific order
        QMap<QString, QString> mDefaultValues; ///< key: locale (like "en_US"), value: default value
        QMap<QString, QString> mPrefixes; ///< key: norm, value: prefix
        QString mDefaultPrefixNorm; ///< must be an existing key of #mPrefixes
        QList<const ComponentSignal*> mSignals; ///< empty if the component has no signals
        QList<const ComponentSymbolVariant*> mSymbolVariants; ///< minimum one entry
        Uuid mDefaultSymbolVariantUuid; ///< must be an existing key of #mSymbolVariants
};

} // namespace library

#endif // LIBRARY_COMPONENT_H
