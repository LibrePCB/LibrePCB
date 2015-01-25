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
#include "../common/filelock.h"
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
class SchematicLayer;

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

        /**
         * @brief Get all Schematic Layers
         *
         * @return A reference to the QHash with all schematic layers
         */
        const QHash<unsigned int, SchematicLayer*>& getSchematicLayers() const noexcept {return mSchematicLayers;}

        /**
         * @brief Get a Schematic Layer with a specific ID
         *
         * @param id    The ID of the layer
         *
         * @return  A pointer to the SchematicLayer object, or NULL if there is no layer
         *          with the specified ID
         */
        SchematicLayer* getSchematicLayer(unsigned int id) const noexcept {return mSchematicLayers.value(id, 0);}


        // Project Management

        /**
         * @brief Create a new project and open it
         *
         * @param filepath  The filepath to the *.e4u project file of the project to create
         *
         * @return  The pointer to the new project. If an error occurs, this method shows
         *          the error message box and returns nullptr afterwards.
         */
        project::Project* createProject(const FilePath& filepath) noexcept;

        /**
         * @brief Open an existing project (or bring an already opened project to front)
         *
         * @param filepath  The filepath to the *.e4u project file to open
         *
         * @return  The pointer to the opened project. If an error occurs, this method
         *          shows the error message box and returns nullptr afterwards.
         */
        project::Project* openProject(const FilePath& filepath) noexcept;

        /**
         * @brief Close an open project
         *
         * @param project       The pointer to the open project
         * @param askForSave    If true and the specified project has unsaved changes,
         *                      a message box appears to ask whether the project should
         *                      be saved or not.
         *                      If false, the project will be closed without saving it.
         *
         * @return  False if the user has canceled the "save project?" dialog (if appeared).
         *          True in all other cases (also if the specified project was not open).
         */
        bool closeProject(project::Project* project, bool askForSave) noexcept;

        /**
         * @overload
         * @brief Close an open project
         *
         * @param filepath      The filepath to the open project
         * @param askForSave    See #closeProject(project::Project*, bool)
         *
         * @return See #closeProject(project::Project*, bool)
         */
        bool closeProject(const FilePath& filepath, bool askForSave) noexcept;

        /**
         * @brief Close all open projects
         *
         * @param askForSave    See #closeProject(project::Project*, bool)
         *
         * @return  False if the user has canceled at least one "save project?" dialog (so
         *          at least one project is still open after calling this method).
         *          True in all other cases.
         */
        bool closeAllProjects(bool askForSave = false) noexcept;

        /**
         * @brief Method to unregister an open project
         *
         * @warning This method must be called only from the destructor project#Project#~Project()!
         *
         * @param project   The pointer to the project which will be closed
         */
        void unregisterOpenProject(project::Project* project) noexcept;

        /**
         * @brief Get the pointer to an already open project by its filepath
         *
         * This method can also be used to check whether a project (by its filepath) is
         * already open or not.
         *
         * @param filepath  The filepath to a *.e4u project file
         *
         * @return The pointer to the open project, or nullptr if the project is not open
         */
        project::Project* getOpenProject(const FilePath& filepath) const noexcept;

        /**
         * @brief Check whether a project is in the favorite project list or not
         *
         * @param filepath  The filepath to a *.e4u project file
         *
         * @return True if the specified project is in the favorites list, false otherwise
         */
        bool isFavoriteProject(const FilePath& filepath) const noexcept;

        /**
         * @brief Add a project to the favorite projects list
         *
         * @param filepath  The filepath to a *.e4u project file
         */
        void addFavoriteProject(const FilePath& filepath) noexcept;

        /**
         * @brief Remove a project from the favorite projects list
         *
         * @param filepath  The filepath to a *.e4u project file
         */
        void removeFavoriteProject(const FilePath& filepath) noexcept;


        // Static Methods

        /**
         * @brief Check whether a filepath points to a valid workspace directory or not
         *
         * @param path  A path to a directory
         *
         * @return True if the path is a valid workspace directory, false otherwise
         */
        static bool isValidWorkspacePath(const FilePath& path) noexcept;

        /**
         * @brief Create a new workspace
         *
         * @param path  A path to a directory where to create the new workspace
         *
         * @return True if success, false otherwise
         */
        static bool createNewWorkspace(const FilePath& path) noexcept;

        /**
         * @brief Get the most recently used workspace path
         *
         * @return The filepath to the last recently used workspace (may be invalid)
         */
        static FilePath getMostRecentlyUsedWorkspacePath() noexcept;

        /**
         * @brief Set the most recently used workspace path
         *
         * @param path  The filepath to the workspace directory
         */
        static void setMostRecentlyUsedWorkspacePath(const FilePath& path) noexcept;

        /**
         * @brief Let the user choose a workspace path (with a directory chooser dialog)
         *
         * @return The choosen filepath (is invalid on error or user cancel)
         */
        static FilePath chooseWorkspacePath() noexcept;

        /**
         * @brief Get the Workspace singleton instance
         *
         * @warning You must be sure that the singleton Workspace object was already
         *          created before calling this method the first time! Otherwise the
         *          application will crash (for debugging purposes there is a Q_ASSERT()
         *          in this method to detect such an error).
         *
         * @return A reference to the singleton Workspace object
         */
        static Workspace& instance() noexcept {Q_ASSERT(sInstance); return *sInstance;}


    public slots:

        // Public Slots

        /**
         * @brief Show the control panel (bring it to front if already open)
         */
        void showControlPanel() const noexcept;

        /**
         * @brief Open the library editor (bring it to front if already open)
         */
        void openLibraryEditor() noexcept;


    private:

        // make some methods inaccessible...
        Workspace();
        Workspace(const Workspace& other);
        Workspace& operator=(const Workspace& rhs);


        // Attributes
        FilePath mPath; ///< a FilePath object which represents the workspace directory
        FileLock mLock; ///< to lock the whole workspace (allow only one app instance)
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
        QHash<unsigned int, SchematicLayer*> mSchematicLayers; ///< all workspace schematic layers


        // static variables
        static Workspace* sInstance;
};

#endif // WORKSPACE_H
