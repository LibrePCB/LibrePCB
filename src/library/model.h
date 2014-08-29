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

#ifndef LIBRARY_MODEL_H
#define LIBRARY_MODEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "libraryelement.h"

/*****************************************************************************************
 *  Class Model
 ****************************************************************************************/

namespace library {

/**
 * @brief The Model class
 */
class Model : public LibraryElement
{
        Q_OBJECT

    public:

        explicit Model(Workspace* workspace, const FilePath& xmlFilePath);
        virtual ~Model();

    private:

        // make some methods inaccessible...
        Model();
        Model(const Model& other);
        Model& operator=(const Model& rhs);

};

} // namespace library

#endif // LIBRARY_MODEL_H
