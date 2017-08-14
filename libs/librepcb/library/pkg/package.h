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

#ifndef LIBREPCB_LIBRARY_PACKAGE_H
#define LIBREPCB_LIBRARY_PACKAGE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../libraryelement.h"
#include "packagepad.h"
#include "footprint.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Package
 ****************************************************************************************/

/**
 * @brief The Package class
 */
class Package final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Package() = delete;
        Package(const Package& other) = delete;
        Package(const Uuid& uuid, const Version& version, const QString& author,
                const QString& name_en_US, const QString& description_en_US,
                const QString& keywords_en_US);
        Package(const FilePath& elementDirectory, bool readOnly);
        ~Package() noexcept;

        // PackagePad Methods
        const QMap<Uuid, PackagePad*>& getPads() noexcept {return mPads;}
        QList<Uuid> getPadUuids() const noexcept {return mPads.keys();}
        PackagePad* getPadByUuid(const Uuid& uuid) noexcept {return mPads.value(uuid);}
        const PackagePad* getPadByUuid(const Uuid& uuid) const noexcept {return mPads.value(uuid);}
        void addPad(PackagePad& pad) noexcept;
        void removePad(PackagePad& pad) noexcept;

        // Footprint Methods
        const QMap<Uuid, Footprint*>& getFootprints() noexcept {return mFootprints;}
        QList<Uuid> getFootprintUuids() const noexcept {return mFootprints.keys();}
        const Footprint* getFootprintByUuid(const Uuid& uuid) const noexcept {return mFootprints.value(uuid);}
        Footprint* getFootprintByUuid(const Uuid& uuid) noexcept {return mFootprints.value(uuid);}
        void addFootprint(Footprint& footprint) noexcept;
        void removeFootprint(Footprint& footprint) noexcept;

        // Operator Overloadings
        Package& operator=(const Package& rhs) = delete;

        // Static Methods
        static QString getShortElementName() noexcept {return QStringLiteral("pkg");}
        static QString getLongElementName() noexcept {return QStringLiteral("package");}


    private:

        // Private Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const override;
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        QMap<Uuid, PackagePad*> mPads; ///< empty if the package has no pads
        QMap<Uuid, Footprint*> mFootprints; ///< minimum one footprint
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_PACKAGE_H
