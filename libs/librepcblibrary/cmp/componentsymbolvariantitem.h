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

#ifndef LIBRARY_COMPONENTSYMBOLVARIANTITEM_H
#define LIBRARY_COMPONENTSYMBOLVARIANTITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class ComponentSymbolVariantItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The ComponentSymbolVariantItem class
 */
class ComponentSymbolVariantItem final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentSymbolVariantItem)

    public:

        /// Pin Display Type Enum
        enum class PinDisplayType_t {
            None,
            PinName,
            ComponentSignal,
            NetSignal
        };

        /// Pin-Signal-Map item struct
        struct PinSignalMapItem_t {
            Uuid pin;                      ///< must be valid
            Uuid signal;                   ///< NULL if not connected to a signal
            PinDisplayType_t displayType;
        };

        // Constructors / Destructor
        explicit ComponentSymbolVariantItem(const Uuid& uuid = Uuid::createRandom(),
                                            const Uuid& symbolUuid = Uuid(),
                                            bool isRequired = false,
                                            const QString& suffix = QString()) noexcept;
        explicit ComponentSymbolVariantItem(const XmlDomElement& domElement) throw (Exception);
        ~ComponentSymbolVariantItem() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Uuid& getSymbolUuid() const noexcept {return mSymbolUuid;}
        bool isRequired() const noexcept {return mIsRequired;}
        const QString& getSuffix() const noexcept {return mSuffix;}

        // Getters: Pin-Signal-Map
        const QHash<Uuid, PinSignalMapItem_t>& getPinSignalMap() const noexcept {return mPinSignalMap;}
        Uuid getSignalOfPin(const Uuid& pinUuid) const noexcept;
        PinDisplayType_t getDisplayTypeOfPin(const Uuid& pinUuid) const noexcept;

        // General Methods
        void addPinSignalMapping(const Uuid& pin, const Uuid& signal, PinDisplayType_t display) noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        ComponentSymbolVariantItem(const ComponentSymbolVariantItem& other);
        ComponentSymbolVariantItem& operator=(const ComponentSymbolVariantItem& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Symbol Variant Item Attributes
        Uuid mUuid;
        Uuid mSymbolUuid;
        bool mIsRequired;
        QString mSuffix;
        QHash<Uuid, PinSignalMapItem_t> mPinSignalMap; ///< All pins required!
};

} // namespace library

#endif // LIBRARY_COMPONENTSYMBOLVARIANTITEM_H
