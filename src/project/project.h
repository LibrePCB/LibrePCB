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

#ifndef PROJECT_PROJECT_H
#define PROJECT_PROJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QMainWindow>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace project {
class Circuit;
class SchematicEditor;
}

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

        // General Methods
        bool windowIsAboutToClose(QMainWindow* window);

        // Static Methods
        static QString uniqueProjectFilename(const QString& filename);

    public slots:

        void showSchematicEditor();
        void save();
        bool close(QWidget* messageBoxParent = 0);

    private:

        // make some methods inaccessible...
        Project();
        Project(const Project& other);
        Project& operator=(const Project& rhs);

        Workspace* mWorkspace; ///< a pointer to the workspace
        QString mFilename;

        // General
        bool mHasUnsavedChanges;
        Circuit* mCircuit;
        SchematicEditor* mSchematicEditor;

};

} // namespace project

#endif // PROJECT_PROJECT_H
