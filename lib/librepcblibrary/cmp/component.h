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
        explicit Component(const QUuid& uuid = QUuid::createUuid(),
                           const Version& version = Version(),
                           const QString& author = QString(),
                           const QString& name_en_US = QString(),
                           const QString& description_en_US = QString(),
                           const QString& keywords_en_US = QString()) throw (Exception);
        explicit Component(const FilePath& xmlFilePath) throw (Exception);
        ~Component() noexcept;

        // Getters
        const QUuid& getGenCompUuid() const noexcept {return mGenericComponentUuid;}
        const QUuid& getPackageUuid() const noexcept {return mPackageUuid;}
        const QHash<QUuid, QUuid>& getPadSignalMap() const noexcept {return mPadSignalMap;}
        QUuid getSignalOfPad(const QUuid& pad) const noexcept {return mPadSignalMap.value(pad);}

        // Setters
        void setGenCompUuid(const QUuid& uuid) noexcept {mGenericComponentUuid = uuid;}
        void setPackageUuid(const QUuid& uuid) noexcept {mPackageUuid = uuid;}

        // General Methods
        void clearPadSignalMap() noexcept {mPadSignalMap.clear();}
        void addPadSignalMapping(const QUuid& pad, const QUuid& signal) noexcept {mPadSignalMap.insert(pad, signal);}


    private:

        // make some methods inaccessible...
        Component();
        Component(const Component& other);
        Component& operator=(const Component& rhs);


        // Private Methods

        void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(uint version) const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        QUuid mGenericComponentUuid;
        QUuid mPackageUuid;
        QHash<QUuid, QUuid> mPadSignalMap; ///< key: pad, value: signal
};

} // namespace library

#endif // LIBRARY_COMPONENT_H
