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
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace project {
class ProjectLibrary;
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
 * This class represents a whole project with all the content of its directory:
 *  - circuit with schematics and boards
 *  - the project's library
 *  - project settings
 *  - an schematic editor, board editor and other windows
 *  - ...
 *
 * Project objects will be created in a Workspace object. The constructor of the Project
 * class needs the filepath to a project file. Then the project will be opened. The
 * destructor will close the project (without saving). Use the method #save() to write the
 * whole project to the harddisc.
 *
 * The project uses the "Command Design Pattern" for undo/redo actions. See #mUndoStack.
 */
class Project final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Project(Workspace* workspace, const QString& filename);
        ~Project();

        // Getters
        QString getFilename() const {return mFileInfo.filePath();}
        QString getUniqueFilename() const {return uniqueProjectFilename(getFilename());}
        QDir getDir() const {return mFileInfo.dir();}
        const QUndoStack* getUndoStack() const {return mUndoStack;}

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
        QFileInfo mFileInfo; ///< a QFileinfo object of the project file (*.e4u)

        // General
        ProjectLibrary* mProjectLibrary;
        Circuit* mCircuit;
        SchematicEditor* mSchematicEditor;

        /**
         * @brief The undo stack of all commands applied to the project (used for undo/redo)
         *
         * All applied commands to the project are stored in this stack. Each command is an
         * object which inherits from QUndoCommand. The stack is also used to determine if
         * the state of the project has changed since it was saved the last time.
         *
         * @warning Each change to the whole project must be implemented as a QUndoCommand
         * and must be pushed to this undo stack! Other changes will not activate the save
         * action in the toolbar, and maybe cause other problems. Only changes not related
         * to the circuit/board/schematic/..., like project/user-specific settings, cached
         * data, window positions and so on, do not need to be pushed to the undo stack as
         * an undo command does not make sense on such changes.
         *
         * @note For more information about the whole undo/redo concept, read the documentation
         * "Overview of Qt's Undo Framework" (http://qt-project.org/doc/qt-5/qundo.html)
         * and other documentations about the "Command Design Pattern".
         */
        QUndoStack* mUndoStack;

};

} // namespace project

#endif // PROJECT_PROJECT_H
