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

#ifndef PROJECT_GENERICCOMPONENTINSTANCE_H
#define PROJECT_GENERICCOMPONENTINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace project {
class Project;
class Circuit;
}

/*****************************************************************************************
 *  Class GenericComponentInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenericComponentInstance class
 */
class GenericComponentInstance : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenericComponentInstance(Workspace* workspace, Project* project,
                                          Circuit* circuit);
        ~GenericComponentInstance();

    private:

        // make some methods inaccessible...
        GenericComponentInstance();
        GenericComponentInstance(const GenericComponentInstance& other);
        GenericComponentInstance& operator=(const GenericComponentInstance& rhs);

        // General
        Workspace* mWorkspace;
        Project* mProject;
        Circuit* mCircuit;

};

} // namespace project

#endif // PROJECT_GENERICCOMPONENTINSTANCE_H
