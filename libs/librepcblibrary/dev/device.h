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

#ifndef LIBREPCB_LIBRARY_DEVICE_H
#define LIBREPCB_LIBRARY_DEVICE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../libraryelement.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Device
 ****************************************************************************************/

/**
 * @brief The Device class
 */
class Device final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Device(const Uuid& uuid, const Version& version, const QString& author,
                        const QString& name_en_US, const QString& description_en_US,
                        const QString& keywords_en_US) throw (Exception);
        explicit Device(const FilePath& elementDirectory) throw (Exception);
        ~Device() noexcept;

        // Getters
        const Uuid& getComponentUuid() const noexcept {return mComponentUuid;}
        const Uuid& getPackageUuid() const noexcept {return mPackageUuid;}

        // Setters
        void setComponentUuid(const Uuid& uuid) noexcept {mComponentUuid = uuid;}
        void setPackageUuid(const Uuid& uuid) noexcept {mPackageUuid = uuid;}

        // Pad-Signal-Map Methods
        const QHash<Uuid, Uuid>& getPadSignalMap() const noexcept {return mPadSignalMap;}
        Uuid getSignalOfPad(const Uuid& pad) const noexcept {return mPadSignalMap.value(pad);}
        void addPadSignalMapping(const Uuid& pad, const Uuid& signal) noexcept;
        void removePadSignalMapping(const Uuid& pad) noexcept;


    private:

        // make some methods inaccessible...
        Device();
        Device(const Device& other);
        Device& operator=(const Device& rhs);


        // Private Methods

        void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        Uuid mComponentUuid;
        Uuid mPackageUuid;
        QHash<Uuid, Uuid> mPadSignalMap; ///< key: pad, value: signal
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_DEVICE_H
