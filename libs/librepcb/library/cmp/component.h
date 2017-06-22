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

#ifndef LIBREPCB_LIBRARY_COMPONENT_H
#define LIBREPCB_LIBRARY_COMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/attributes/attribute.h>
#include "../libraryelement.h"
#include "componentsignal.h"
#include "componentsymbolvariant.h"

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
        Component() = delete;
        Component(const Component& other) = delete;
        Component(const Uuid& uuid, const Version& version, const QString& author,
                  const QString& name_en_US, const QString& description_en_US,
                  const QString& keywords_en_US) throw (Exception);
        Component(const FilePath& elementDirectory, bool readOnly) throw (Exception);
        ~Component() noexcept;

        // General
        bool isSchematicOnly() const noexcept {return mSchematicOnly;}
        void setIsSchematicOnly(bool schematicOnly) noexcept {mSchematicOnly = schematicOnly;}

        // Attribute Methods
        const AttributeList& getAttributes() const noexcept {return *mAttributes;}
        void setAttributes(const AttributeList& attributes) noexcept {*mAttributes = attributes;}

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

        // Operator Overloadings
        Component& operator=(const Component& rhs) = delete;

        // Static Methods
        static QString getShortElementName() noexcept {return QStringLiteral("cmp");}
        static QString getLongElementName() noexcept {return QStringLiteral("component");}


    private:

        // Private Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;
        bool checkAttributesValidity() const noexcept override;


        // Conponent Attributes
        bool mSchematicOnly; ///< if true, this component is schematic-only (no package)
        QScopedPointer<AttributeList> mAttributes; ///< all attributes in a specific order
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
