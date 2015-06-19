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

#ifndef LIBRARY_MODEL3D_H
#define LIBRARY_MODEL3D_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"

/*****************************************************************************************
 *  Class Model3D
 ****************************************************************************************/

namespace library {

/**
 * @brief The Model3D class
 */
class Model3D final : public LibraryElement
{
        Q_OBJECT

    public:

        explicit Model3D(const FilePath& xmlFilePath);
        virtual ~Model3D();

    private:

        // make some methods inaccessible...
        Model3D();
        Model3D(const Model3D& other);
        Model3D& operator=(const Model3D& rhs);


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);

};

} // namespace library

#endif // LIBRARY_MODEL3D_H
