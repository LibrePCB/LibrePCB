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
#include "../common/exceptions.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class ControlPanel;
class ProjectTreeModel;
class RecentProjectsModel;
class FavoriteProjectsModel;
class WorkspaceSettings;

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
 * @brief The Workspace class represents a workspace with all its data (library, projects,
 * settings, ...)
 *
 * There can be only one Workspace object in an application instance. That object is
 * created in the main() function and is like the "topmost class of the whole application".
 *
 * To access the settings of the workspace, use the method #getSettings().
 *
 * @author ubruhin
 *
 * @date 2014-06-23
 */
class Workspace final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief Constructor to open an existing workspace
         *
         * @param wsPath    The filepath to the workspace directory
         *
         * @throw Exception If the workspace could not be opened, this constructor throws
         *                  an exception.
         */
        explicit Workspace(const FilePath& wsPath) throw (Exception);

        /**
         * The destructor
         */
        ~Workspace();


        // Getters

        /**
         * @brief Get the filepath to the workspace directory
         */
        const FilePath& getPath() const {return mPath;}

        /**
         * @brief Get the filepath to the ".metadata" directory in the workspace
         */
        const FilePath& getMetadataPath() const {return mMetadataPath;}

        /**
         * @brief Get the filepath to the "projects" directory in the workspace
         */
        const FilePath& getProjectsPath() const {return mProjectsPath;}

        /**
         * @brief Get the filepath to the "library" directory in the workspace
         */
        const FilePath& getLibraryPath() const {return mLibraryPath;}

        /**
         * @brief Get the workspace settings
         */
        WorkspaceSettings& getSettings() const {return *mWorkspaceSettings;}

        /**
         * @brief Get the workspace library
         */
        library::Library& getLibrary() const {return *mLibrary;}


        // Project Management
        project::Project* openProject(const FilePath& filepath) noexcept;
        bool closeProject(const FilePath& filepath, bool askForSave);
        bool closeProject(project::Project* project, bool askForSave);
        void unregisterOpenProject(project::Project* project);
        project::Project* getOpenProject(const FilePath& filepath);
        bool isFavoriteProject(const FilePath& filepath) const;
        void addFavoriteProject(const FilePath& filepath);
        void removeFavoriteProject(const FilePath& filepath);

        // Static Methods
        static bool isValidWorkspacePath(const FilePath& path);
        static bool createNewWorkspace(const FilePath& path);
        static FilePath getMostRecentlyUsedWorkspacePath();
        static void setMostRecentlyUsedWorkspacePath(const FilePath& path);
        static FilePath chooseWorkspacePath();

    public slots:

        void showControlPanel() const;
        void openLibraryEditor();
        void closeAllProjects(bool askForSave = false);

    private:

        // make some methods inaccessible...
        Workspace();
        Workspace(const Workspace& other);
        Workspace& operator=(const Workspace& rhs);

        FilePath mPath; ///< a FilePath object which represents the workspace directory
        FilePath mMetadataPath; ///< the directory ".metadata"
        FilePath mProjectsPath; ///< the directory "projects"
        FilePath mLibraryPath; ///< the directory "library"
        WorkspaceSettings* mWorkspaceSettings; ///< the WorkspaceSettings object
        ControlPanel* mControlPanel; ///< the control panel window
        library::Library* mLibrary; ///< the library of the workspace (with SQLite database)
        library_editor::LibraryEditor* mLibraryEditor; ///< the library editor
        ProjectTreeModel* mProjectTreeModel; ///< a tree model for the whole projects directory
        QHash<QString, project::Project*> mOpenProjects; ///< a list of all open projects
        RecentProjectsModel* mRecentProjectsModel; ///< a list model of all recent projects
        FavoriteProjectsModel* mFavoriteProjectsModel; ///< a list model of all favorite projects
};

#endif // WORKSPACE_H
