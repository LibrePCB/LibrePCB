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

#ifndef PROJECT_UNPLACEDSYMBOLSDOCK_H
#define PROJECT_UNPLACEDSYMBOLSDOCK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
}

namespace Ui {
class UnplacedSymbolsDock;
}

/*****************************************************************************************
 *  Class UnplacedSymbolsDock
 ****************************************************************************************/

namespace project {

/**
 * @brief The UnplacedSymbolsDock class
 */
class UnplacedSymbolsDock final : public QDockWidget
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit UnplacedSymbolsDock(Project& project);
        ~UnplacedSymbolsDock();

    private:

        // make some methods inaccessible...
        UnplacedSymbolsDock();
        UnplacedSymbolsDock(const UnplacedSymbolsDock& other);
        UnplacedSymbolsDock& operator=(const UnplacedSymbolsDock& rhs);

        // General
        Project& mProject;
        Ui::UnplacedSymbolsDock* mUi;
};

} // namespace project

#endif // PROJECT_UNPLACEDSYMBOLSDOCK_H
