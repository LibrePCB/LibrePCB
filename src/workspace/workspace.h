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
class RecentProjectsModel;
class FavoriteProjectsModel;

namespace library{
class Library;
}

namespace library_editor{
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
        QString getWorkspaceSettingsIniFilename() const {return mWorkspaceSettings->fileName();}
        library::Library* getLibrary() const {return mLibrary;}

        // Project Management
        project::Project* openProject(const QString& filename);
        bool closeProject(const QString& filename, bool askForSave);
        bool closeProject(project::Project* project, bool askForSave);
        void unregisterOpenProject(project::Project* project);
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

        // make some methods inaccessible...
        Workspace();
        Workspace(const Workspace& other);
        Workspace& operator=(const Workspace& rhs);

        QDir mWorkspaceDir;
        QDir mMetadataDir;
        QSettings* mWorkspaceSettings;

        ControlPanel* mControlPanel;
        library::Library* mLibrary;
        library_editor::LibraryEditor* mLibraryEditor;
        ProjectTreeModel* mProjectTreeModel;
        QHash<QString, project::Project*> mOpenProjects;
        RecentProjectsModel* mRecentProjectsModel;
        FavoriteProjectsModel* mFavoriteProjectsModel;
};

#endif // WORKSPACE_H
