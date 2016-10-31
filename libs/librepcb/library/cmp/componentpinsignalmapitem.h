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

#ifndef LIBREPCB_LIBRARY_COMPONENTPINSIGNALMAPITEM_H
#define LIBREPCB_LIBRARY_COMPONENTPINSIGNALMAPITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class ComponentPinSignalMapItem
 ****************************************************************************************/

/**
 * @brief The ComponentPinSignalMapItem class
 */
class ComponentPinSignalMapItem final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentPinSignalMapItem)

    public:

        /// Pin Display Type Enum
        enum class PinDisplayType_t {
            NONE,               ///< no text
            PIN_NAME,           ///< display the name of the symbol pin
            COMPONENT_SIGNAL,   ///< display the name of the component signal
            NET_SIGNAL          ///< display the name of the connected net signal
        };

        // Constructors / Destructor
        explicit ComponentPinSignalMapItem(const Uuid& pin, const Uuid& signal,
                                           PinDisplayType_t displayType) noexcept;
        explicit ComponentPinSignalMapItem(const XmlDomElement& domElement) throw (Exception);
        ~ComponentPinSignalMapItem() noexcept;

        // Getters
        const Uuid& getPinUuid() const noexcept {return mPinUuid;}
        const Uuid& getSignalUuid() const noexcept {return mSignalUuid;}
        PinDisplayType_t getDisplayType() const noexcept {return mDisplayType;}

        // Setters
        void setSignalUuid(const Uuid& uuid) noexcept {mSignalUuid = uuid;}
        void setDisplayType(PinDisplayType_t type) noexcept {mDisplayType = type;}

        /// @copydoc #IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Static Methods
        static PinDisplayType_t stringToDisplayType(const QString& type) throw (Exception);
        static QString displayTypeToString(PinDisplayType_t type) noexcept;


    private:

        // make some methods inaccessible...
        ComponentPinSignalMapItem() = delete;
        ComponentPinSignalMapItem(const ComponentPinSignalMapItem& other) = delete;
        ComponentPinSignalMapItem& operator=(const ComponentPinSignalMapItem& rhs) = delete;

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        Uuid mPinUuid;                      ///< must be valid
        Uuid mSignalUuid;                   ///< NULL if not connected to a signal
        PinDisplayType_t mDisplayType;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENTPINSIGNALMAPITEM_H
