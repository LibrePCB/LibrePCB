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

#ifndef LIBRARY_PACKAGE_H
#define LIBRARY_PACKAGE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"
#include "packagepad.h"
#include "footprint.h"

/*****************************************************************************************
 *  Class Package
 ****************************************************************************************/

namespace library {

/**
 * @brief The Package class
 */
class Package final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Package(const Uuid& uuid = Uuid::createRandom(),
                         const Version& version = Version(),
                         const QString& author = QString(),
                         const QString& name_en_US = QString(),
                         const QString& description_en_US = QString(),
                         const QString& keywords_en_US = QString()) throw (Exception);
        explicit Package(const FilePath& elementDirectory) throw (Exception);
        ~Package() noexcept;

        // Pads
        const QList<const PackagePad*>& getPads() const noexcept {return mPads;}
        const PackagePad* getPadByUuid(const Uuid& uuid) const noexcept;
        void clearPads() noexcept;
        void addPad(const PackagePad& pad) noexcept;

        // Footprints
        const QList<const Footprint*>& getFootprints() const noexcept {return mFootprints;}
        const Footprint* getFootprintByUuid(const Uuid& uuid) const noexcept;
        void clearFootprints() noexcept;
        void addFootprint(const Footprint& footprint) noexcept;


    private:

        // make some methods inaccessible...
        Package() = delete;
        Package(const Package& other) = delete;
        Package& operator=(const Package& rhs) = delete;


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        QList<const PackagePad*> mPads; ///< empty if the package has no pads
        QList<const Footprint*> mFootprints; ///< minimum one footprint
};

} // namespace library

#endif // LIBRARY_PACKAGE_H
