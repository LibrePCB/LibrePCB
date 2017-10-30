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

#ifndef LIBREPCB_LIBRARY_PACKAGEPAD_H
#define LIBREPCB_LIBRARY_PACKAGEPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class PackagePad
 ****************************************************************************************/

/**
 * @brief The PackagePad class represents one logical pad of a package
 *
 * Following information is considered as the "interface" of a pad and must therefore
 * never be changed:
 *  - UUID
 */
class PackagePad final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(PackagePad)

    public:

        // Constructors / Destructor
        PackagePad() = delete;
        PackagePad(const PackagePad& other) noexcept;
        PackagePad(const Uuid& uuid, const QString& name) noexcept;
        explicit PackagePad(const SExpression& node);
        ~PackagePad() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        QString getName() const noexcept {return mName;}

        // Setters
        void setName(const QString& name) noexcept;

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const PackagePad& rhs) const noexcept;
        bool operator!=(const PackagePad& rhs) const noexcept {return !(*this == rhs);}
        PackagePad& operator=(const PackagePad& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        QString mName;
};

/*****************************************************************************************
 *  Class PackagePadList
 ****************************************************************************************/

struct PackagePadListNameProvider {static constexpr const char* tagname = "pad";};
using PackagePadList = SerializableObjectList<PackagePad, PackagePadListNameProvider>;
using CmdPackagePadInsert = CmdListElementInsert<PackagePad, PackagePadListNameProvider>;
using CmdPackagePadRemove = CmdListElementRemove<PackagePad, PackagePadListNameProvider>;
using CmdPackagePadsSwap = CmdListElementsSwap<PackagePad, PackagePadListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_PACKAGEPAD_H
