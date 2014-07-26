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

#ifndef PROJECT_SCHEMATIC_H
#define PROJECT_SCHEMATIC_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace project {
class Project;
class Circuit;
}

/*****************************************************************************************
 *  Class Schematic
 ****************************************************************************************/

namespace project {

/**
 * @brief The Schematic class
 */
class Schematic final : public QObject
{
        Q_OBJECT

    public:

        explicit Schematic(Workspace* workspace, Project* project, Circuit* circuit);
        ~Schematic();

    private:

        // make some methods inaccessible...
        Schematic();
        Schematic(const Schematic& other);
        Schematic& operator=(const Schematic& rhs);

        // General
        Workspace* mWorkspace; ///< A pointer to the Workspace object (from the constructor)
        Project* mProject; ///< A pointer to the Project object (from the constructor)
        Circuit* mCircuit; ///< A pointer to the Circuit object (from the constructor)

};

} // namespace project

#endif // PROJECT_SCHEMATIC_H
