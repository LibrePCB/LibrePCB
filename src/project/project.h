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

#ifndef PROJECT_H
#define PROJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

/*****************************************************************************************
 *  Class Project
 ****************************************************************************************/

namespace project {

/**
 * @brief The Project class
 *
 * @todo this is only a stub class...
 */
class Project : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Project(Workspace* workspace, const QString& filename);
        ~Project();

        // Getters
        const QString& getFilename() const {return mFilename;}
        QString getUniqueFilename() const {return uniqueProjectFilename(mFilename);}

        // Static Methods
        static QString uniqueProjectFilename(const QString& filename);

    private:

        // make the default constructor and the copy constructor inaccessable
        Project();
        Project(const Project& other) : QObject(0) {Q_UNUSED(other);}

        Workspace* mWorkspace; ///< a pointer to the workspace
        QString mFilename;

};

} // namespace library

#endif // PROJECT_H
