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

#ifndef LIBRARY_LIBRARYCATEGORY_H
#define LIBRARY_LIBRARYCATEGORY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../librarybaseelement.h"

/*****************************************************************************************
 *  Class LibraryCategory
 ****************************************************************************************/

namespace library {

/**
 * @brief The LibraryCategory class extends the LibraryBaseElement class with some
 *        attributes and methods which are used for all library category classes.
 */
class LibraryCategory : public LibraryBaseElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit LibraryCategory(const QString& xmlFileNamePrefix,
                                 const QString& xmlRootNodeName,
                                 const QUuid& uuid = QUuid::createUuid(),
                                 const Version& version = Version(),
                                 const QString& author = QString(),
                                 const QString& name_en_US = QString(),
                                 const QString& description_en_US = QString(),
                                 const QString& keywords_en_US = QString()) throw (Exception);
        explicit LibraryCategory(const FilePath& elementDirectory,
                                 const QString& xmlFileNamePrefix,
                                 const QString& xmlRootNodeName) throw (Exception);
        virtual ~LibraryCategory() noexcept;

        // Getters: Attributes
        const QUuid& getParentUuid() const noexcept {return mParentUuid;}

        // Setters: Attributes
        void setParentUuid(const QUuid& parentUuid) noexcept {mParentUuid = parentUuid;}


    private:

        // make some methods inaccessible...
        LibraryCategory(const LibraryCategory& other);
        LibraryCategory& operator=(const LibraryCategory& rhs);


    protected:

        // Protected Methods
        virtual void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        virtual XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        virtual bool checkAttributesValidity() const noexcept override;

        // General Library Category Attributes
        QUuid mParentUuid;
};

} // namespace library

#endif // LIBRARY_LIBRARYCATEGORY_H
