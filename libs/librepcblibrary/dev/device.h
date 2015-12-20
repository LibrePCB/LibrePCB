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

#ifndef LIBRARY_DEVICE_H
#define LIBRARY_DEVICE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"

/*****************************************************************************************
 *  Class Device
 ****************************************************************************************/

namespace library {

/**
 * @brief The Device class
 */
class Device final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Device(const QUuid& uuid = QUuid::createUuid(),
                        const Version& version = Version(),
                        const QString& author = QString(),
                        const QString& name_en_US = QString(),
                        const QString& description_en_US = QString(),
                        const QString& keywords_en_US = QString()) throw (Exception);
        explicit Device(const FilePath& elementDirectory) throw (Exception);
        ~Device() noexcept;

        // Getters
        const QUuid& getComponentUuid() const noexcept {return mComponentUuid;}
        const QUuid& getPackageUuid() const noexcept {return mPackageUuid;}
        const QHash<QUuid, QUuid>& getPadSignalMap() const noexcept {return mPadSignalMap;}
        QUuid getSignalOfPad(const QUuid& pad) const noexcept {return mPadSignalMap.value(pad);}

        // Setters
        void setComponentUuid(const QUuid& uuid) noexcept {mComponentUuid = uuid;}
        void setPackageUuid(const QUuid& uuid) noexcept {mPackageUuid = uuid;}

        // General Methods
        void clearPadSignalMap() noexcept {mPadSignalMap.clear();}
        void addPadSignalMapping(const QUuid& pad, const QUuid& signal) noexcept {mPadSignalMap.insert(pad, signal);}


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
        QUuid mComponentUuid;
        QUuid mPackageUuid;
        QHash<QUuid, QUuid> mPadSignalMap; ///< key: pad, value: signal
};

} // namespace library

#endif // LIBRARY_DEVICE_H
