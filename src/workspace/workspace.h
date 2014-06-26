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

#ifndef WORKSPACE_H
#define WORKSPACE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class ControlPanel;
class ProjectTreeModel;

namespace library{
class Library;
}

namespace libedit{
class LibraryEditor;
}

namespace project{
class Project;
}

/*****************************************************************************************
 *  Class Workspace
 ****************************************************************************************/

/**
 * @brief The Workspace class
 *
 * @author ubruhin
 *
 * @date 2014-06-23
 */
class Workspace : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Workspace(const QDir& workspaceDir);
        virtual ~Workspace();

        // Getters
        const QDir& getWorkspaceDir() const {return mWorkspaceDir;}
        QString getUniquePath() const {return uniqueWorkspacePath(mWorkspaceDir.absolutePath());}
        const QDir& getMetadataDir() const {return mMetadataDir;}
        const QSettings& getWorkspaceSettings() const {return *mWorkspaceSettings;}
        library::Library* getLibrary() const {return mLibrary;}

        // Project Management
        project::Project* openProject(const QString& filename);
        void closeProject(const QString& filename);
        void closeProject(project::Project* project);
        project::Project* getOpenProject(const QString& filename);
        const QHash<QString, project::Project*>& getOpenProjects() const {return mOpenProjects;}

        // Static Methods
        static bool isValidWorkspaceDir(const QDir& dir);
        static void createNewWorkspace(const QDir& dir);
        static QString getMostRecentlyUsedWorkspacePath();
        static void setMostRecentlyUsedWorkspacePath(const QString& path);
        static QStringList getAllWorkspacePaths();
        static void setAllWorkspacePaths(const QStringList& paths);
        static QString uniqueWorkspacePath(const QString& path);

    public slots:

        void showControlPanel() const;
        void openLibraryEditor();
        void closeAllProjects(bool askForSave = false);

    private:

        // make the default constructor and the copy constructor inaccessable
        Workspace();
        Workspace(const Workspace& other) : QObject(0) {Q_UNUSED(other);}

        QDir mWorkspaceDir;
        QDir mMetadataDir;
        QSettings* mWorkspaceSettings;

        ControlPanel* mControlPanel;
        library::Library* mLibrary;
        libedit::LibraryEditor* mLibraryEditor;
        ProjectTreeModel* mProjectTreeModel;
        QHash<QString, project::Project*> mOpenProjects;

};

#endif // WORKSPACE_H
