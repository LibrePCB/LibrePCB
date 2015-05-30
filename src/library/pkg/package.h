/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
        explicit Package(const FilePath& xmlFilePath) throw (Exception);
        virtual ~Package() noexcept;

        // Getters
        const QUuid& getFootprintUuid() const noexcept {return mFootprintUuid;}


    private:

        // make some methods inaccessible...
        Package() = delete;
        Package(const Package& other) = delete;
        Package& operator=(const Package& rhs) = delete;


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // Attributes
        QUuid mFootprintUuid;
};

} // namespace library

#endif // LIBRARY_PACKAGE_H
