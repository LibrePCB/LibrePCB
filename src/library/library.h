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

#ifndef LIBRARY_LIBRARY_H
#define LIBRARY_LIBRARY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtSql>

#include "../common/exceptions.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Class Library
 ****************************************************************************************/

namespace library {

/**
 * @brief The Library class
 *
 * @todo this is only a stub class...
 */
class Library : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
        * @brief Constructor to open the library in the existing workspace
        *
        * @param libPath    The filepath to the library directory
        *
        * @throw Exception If the library could not be opened, this constructor throws
        *                  an exception.
        */
        explicit Library(const FilePath& libPath) throw (Exception);
        ~Library();

    private:

        // make some methods inaccessible...
        Library();
        Library(const Library& other);
        Library& operator=(const Library& rhs);

        FilePath mLibPath; ///< a FilePath object which represents the library directory
        FilePath mLibFilePath; ///<a FiltePath object which represents the lib.db-file
        QSqlDatabase mLibDatabase; ///<a QSqlDatabase object which contents the lib.db-file
};

} // namespace library

#endif // LIBRARY_LIBRARY_H
