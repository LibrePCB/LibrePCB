/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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

#ifndef LIBREPCB_WORKSPACE_H
#define LIBREPCB_WORKSPACE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/fileio/filelock.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library{
class Library;
}

namespace project{
class Project;
}

namespace workspace {

class ProjectTreeModel;
class RecentProjectsModel;
class FavoriteProjectsModel;
class WorkspaceSettings;

/*****************************************************************************************
 *  Class Workspace
 ****************************************************************************************/

/**
 * @brief The Workspace class represents a workspace with all its data (library, projects,
 * settings, ...)
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

        QAbstractItemModel& getProjectTreeModel() const noexcept;
        QAbstractItemModel& getRecentProjectsModel() const noexcept;
        QAbstractItemModel& getFavoriteProjectsModel() const noexcept;

        /**
         * @brief Get the workspace settings
         */
        WorkspaceSettings& getSettings() const {return *mWorkspaceSettings;}

        /**
         * @brief Get the workspace library
         */
        library::Library& getLibrary() const {return *mLibrary;}


        // Project Management

        /**
         * @brief setLastRecentlyUsedProject
         * @param filepath
         */
        void setLastRecentlyUsedProject(const FilePath& filepath) noexcept;

        /**
         * @brief Check whether a project is in the favorite project list or not
         *
         * @param filepath  The filepath to a *.lpp project file
         *
         * @return True if the specified project is in the favorites list, false otherwise
         */
        bool isFavoriteProject(const FilePath& filepath) const noexcept;

        /**
         * @brief Add a project to the favorite projects list
         *
         * @param filepath  The filepath to a *.lpp project file
         */
        void addFavoriteProject(const FilePath& filepath) noexcept;

        /**
         * @brief Remove a project from the favorite projects list
         *
         * @param filepath  The filepath to a *.lpp project file
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


    private:

        // make some methods inaccessible...
        Workspace();
        Workspace(const Workspace& other);
        Workspace& operator=(const Workspace& rhs);


        // Attributes
        FilePath mPath; ///< a FilePath object which represents the workspace directory
        FileLock mLock; ///< to lock the whole workspace (allow only one app instance)
        FilePath mMetadataPath; ///< the directory ".metadata/v#"
        FilePath mProjectsPath; ///< the directory "projects"
        FilePath mLibraryPath; ///< the directory "library"
        WorkspaceSettings* mWorkspaceSettings; ///< the WorkspaceSettings object
        library::Library* mLibrary; ///< the library of the workspace
        ProjectTreeModel* mProjectTreeModel; ///< a tree model for the whole projects directory
        RecentProjectsModel* mRecentProjectsModel; ///< a list model of all recent projects
        FavoriteProjectsModel* mFavoriteProjectsModel; ///< a list model of all favorite projects
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_H
