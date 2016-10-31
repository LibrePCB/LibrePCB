/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <librepcbcommon/fileio/directorylock.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/version.h>
#include <librepcbcommon/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
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
class WorkspaceLibraryDb;

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
        Workspace() = delete;
        Workspace(const Workspace& other) = delete;

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
        ~Workspace() noexcept;


        // Getters

        /**
         * @brief Get the filepath to the workspace directory
         */
        const FilePath& getPath() const {return mPath;}

        /**
         * @brief Get the filepath to the "projects" directory in the workspace
         */
        const FilePath& getProjectsPath() const {return mProjectsPath;}

        /**
         * @brief Get the filepath to the version directory (v#) in the workspace
         */
        const FilePath& getVersionPath() const {return mVersionPath;}

        /**
         * @brief Get the filepath to the "v#/metadata" directory in the workspace
         */
        const FilePath& getMetadataPath() const {return mMetadataPath;}

        /**
         * @brief Get the filepath to the "v#/libraries" directory in the workspace
         */
        const FilePath& getLibrariesPath() const {return mLibrariesPath;}

        QAbstractItemModel& getProjectTreeModel() const noexcept;
        QAbstractItemModel& getRecentProjectsModel() const noexcept;
        QAbstractItemModel& getFavoriteProjectsModel() const noexcept;

        /**
         * @brief Get the workspace settings
         */
        WorkspaceSettings& getSettings() const {return *mWorkspaceSettings;}


        // Library Management

        /**
         * @brief Get the (highest) version of a specific library
         *
         * @param uuid      The uuid of the library
         * @param local     If true, local libraries are searched
         * @param remote    If true, remote libraries are searched
         *
         * @return The highest version of the library (invalid if library not installed)
         */
        Version getVersionOfLibrary(const Uuid& uuid, bool local = true, bool remote = true) const noexcept;

        /**
         * @brief Get all local libraries (located in "workspace/v#/libraries/local")
         *
         * @return A list of all local libraries
         */
        const QMap<QString, QSharedPointer<library::Library>> getLocalLibraries() const noexcept
        {return mLocalLibraries;}

        /**
         * @brief Get all remote libraries (located in "workspace/v#/libraries/remote")
         *
         * @return A list of all remote libraries
         */
        const QMap<QString, QSharedPointer<library::Library>> getRemoteLibraries() const noexcept
        {return mRemoteLibraries;}

        /**
         * @brief Add a new local library
         *
         * @param libDirName    The name of the (existing) local library directory
         *
         * @throws Exception on error
         */
        void addLocalLibrary(const QString& libDirName) throw (Exception);

        /**
         * @brief Add a new remote library
         *
         * @param libDirName    The name of the (existing) remote library directory
         *
         * @throws Exception on error
         */
        void addRemoteLibrary(const QString& libDirName) throw (Exception);

        /**
         * @brief Remove a local library
         *
         * @param libDirName    The name of the (existing) local library directory
         * @param rmDir         It true, the library's directory will be removed
         *
         * @throws Exception on error
         */
        void removeLocalLibrary(const QString& libDirName, bool rmDir = true) throw (Exception);

        /**
         * @brief Remove a remote library
         *
         * @param libDirName    The name of the (existing) remote library directory
         * @param rmDir         It true, the library's directory will be removed
         *
         * @throws Exception on error
         */
        void removeRemoteLibrary(const QString& libDirName, bool rmDir = true) throw (Exception);


        /**
         * @brief Get the workspace library database
         */
        WorkspaceLibraryDb& getLibraryDb() const {return *mLibraryDb;}


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


        // Operator Overloadings
        Workspace& operator=(const Workspace& rhs) = delete;


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
         * @brief getFileFormatVersionsOfWorkspace
         * @param path
         * @return
         */
        static QList<Version> getFileFormatVersionsOfWorkspace(const FilePath& path) noexcept;

        /**
         * @brief getHighestFileFormatVersionOfWorkspace
         * @param path
         * @return
         */
        static Version getHighestFileFormatVersionOfWorkspace(const FilePath& path) noexcept;

        /**
         * @brief Create a new workspace
         *
         * @param path  A path to a directory where to create the new workspace
         *
         * @throws Exception on error.
         */
        static void createNewWorkspace(const FilePath& path) throw (Exception);

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


    signals:

        void libraryAdded(const FilePath& libDir);
        void libraryRemoved(const FilePath& libDir);


    private: // Data

        FilePath mPath; ///< a FilePath object which represents the workspace directory
        FilePath mProjectsPath; ///< the directory "projects"
        FilePath mVersionPath; ///< the subdirectory of the current file format version
        FilePath mMetadataPath; ///< the directory "v#/metadata"
        FilePath mLibrariesPath; ///< the directory "v#/libraries"
        DirectoryLock mLock; ///< to lock the version directory (#mVersionPath)
        QScopedPointer<WorkspaceSettings> mWorkspaceSettings; ///< the WorkspaceSettings object
        QMap<QString, QSharedPointer<library::Library>> mLocalLibraries; ///< all local libraries
        QMap<QString, QSharedPointer<library::Library>> mRemoteLibraries; ///< all remote libraries
        QScopedPointer<WorkspaceLibraryDb> mLibraryDb; ///< the library database
        QScopedPointer<ProjectTreeModel> mProjectTreeModel; ///< a tree model for the whole projects directory
        QScopedPointer<RecentProjectsModel> mRecentProjectsModel; ///< a list model of all recent projects
        QScopedPointer<FavoriteProjectsModel> mFavoriteProjectsModel; ///< a list model of all favorite projects
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_H
