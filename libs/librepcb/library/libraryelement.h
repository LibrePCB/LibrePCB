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

#ifndef LIBREPCB_LIBRARY_LIBRARYELEMENT_H
#define LIBREPCB_LIBRARY_LIBRARYELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "librarybaseelement.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class LibraryElement
 ****************************************************************************************/

/**
 * @brief The LibraryElement class extends the LibraryBaseElement class with some
 *        attributes and methods which are used for all library classes except categories.
 */
class LibraryElement : public LibraryBaseElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryElement() = delete;
        LibraryElement(const LibraryElement& other) = delete;
        LibraryElement(const QString& shortElementName, const QString& longElementName,
                       const Uuid& uuid, const Version& version, const QString& author,
                       const QString& name_en_US, const QString& description_en_US,
                       const QString& keywords_en_US);
        LibraryElement(const FilePath& elementDirectory, const QString& shortElementName,
                       const QString& longElementName, bool readOnly);
        virtual ~LibraryElement() noexcept;

        // Getters: Attributes
        const QSet<Uuid>& getCategories() const noexcept {return mCategories;}

        // Setters: Attributes
        void setCategories(const QSet<Uuid>& uuids) noexcept {mCategories = uuids;}

        // Operator Overloadings
        LibraryElement& operator=(const LibraryElement& rhs) = delete;


    protected:

        // Protected Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        virtual void serialize(SExpression& root) const override;
        virtual bool checkAttributesValidity() const noexcept override;

        // General Library Element Attributes
        QSet<Uuid> mCategories;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_LIBRARYELEMENT_H
