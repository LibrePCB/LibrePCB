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
        explicit LibraryElement(const QString& xmlFileNamePrefix,
                                const QString& xmlRootNodeName,
                                const Uuid& uuid, const Version& version,
                                const QString& author, const QString& name_en_US,
                                const QString& description_en_US,
                                const QString& keywords_en_US) throw (Exception);
        explicit LibraryElement(const FilePath& elementDirectory,
                                const QString& xmlFileNamePrefix,
                                const QString& xmlRootNodeName) throw (Exception);
        virtual ~LibraryElement() noexcept;

        // Getters: Attributes
        const QList<Uuid>& getCategories() const noexcept {return mCategories;}


    private:

        // make some methods inaccessible...
        LibraryElement(const LibraryElement& other);
        LibraryElement& operator=(const LibraryElement& rhs);


    protected:

        // Protected Methods
        virtual void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        virtual XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        virtual bool checkAttributesValidity() const noexcept override;

        // General Library Element Attributes
        QList<Uuid> mCategories;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_LIBRARYELEMENT_H
