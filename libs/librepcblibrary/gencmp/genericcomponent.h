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

#ifndef LIBRARY_GENERICCOMPONENT_H
#define LIBRARY_GENERICCOMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"
#include "gencompsignal.h"
#include "gencompsymbvar.h"
#include "../libraryelementattribute.h"

/*****************************************************************************************
 *  Class GenericComponent
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenericComponent class
 */
class GenericComponent final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenericComponent(const QUuid& uuid = QUuid::createUuid(),
                                  const Version& version = Version(),
                                  const QString& author = QString(),
                                  const QString& name_en_US = QString(),
                                  const QString& description_en_US = QString(),
                                  const QString& keywords_en_US = QString()) throw (Exception);
        explicit GenericComponent(const FilePath& elementDirectory) throw (Exception);
        ~GenericComponent() noexcept;

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
        const QList<const GenCompSignal*>& getSignals() const noexcept;
        const GenCompSignal* getSignalByUuid(const QUuid& uuid) const noexcept;
        const GenCompSignal* getSignalOfPin(const QUuid& symbVarUuid, const QUuid& itemUuid,
                                            const QUuid& pinUuid) const noexcept;
        void clearSignals() noexcept;
        void addSignal(const GenCompSignal& signal) noexcept;

        // Symbol Variants
        const QList<const GenCompSymbVar*>& getSymbolVariants() const noexcept;
        const GenCompSymbVar* getSymbolVariantByUuid(const QUuid& uuid) const noexcept;
        const QUuid& getDefaultSymbolVariantUuid() const noexcept;
        const GenCompSymbVar* getDefaultSymbolVariant() const noexcept;
        void clearSymbolVariants() noexcept;
        void addSymbolVariant(const GenCompSymbVar& symbolVariant) noexcept;

        // Symbol Variant Items
        const GenCompSymbVarItem* getSymbVarItem(const QUuid& symbVarUuid,
                                                 const QUuid& itemUuid) const noexcept;

    private:

        // make some methods inaccessible...
        GenericComponent() = delete;
        GenericComponent(const GenericComponent& other) = delete;
        GenericComponent& operator=(const GenericComponent& rhs) = delete;


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
        QList<const GenCompSignal*> mSignals; ///< empty if the component has no signals
        QList<const GenCompSymbVar*> mSymbolVariants; ///< minimum one entry
        QUuid mDefaultSymbolVariantUuid; ///< must be an existing key of #mSymbolVariants
};

} // namespace library

#endif // LIBRARY_GENERICCOMPONENT_H
