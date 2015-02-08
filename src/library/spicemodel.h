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

#ifndef LIBRARY_SPICEMODEL_H
#define LIBRARY_SPICEMODEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "libraryelement.h"

/*****************************************************************************************
 *  Class SpiceModel
 ****************************************************************************************/

namespace library {

/**
 * @brief The SpiceModel class
 */
class SpiceModel final : public LibraryElement
{
        Q_OBJECT

    public:

        explicit SpiceModel(const FilePath& xmlFilePath);
        virtual ~SpiceModel();

    private:

        // make some methods inaccessible...
        SpiceModel();
        SpiceModel(const SpiceModel& other);
        SpiceModel& operator=(const SpiceModel& rhs);


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);

};

} // namespace library

#endif // LIBRARY_SPICEMODEL_H
