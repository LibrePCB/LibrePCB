/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#ifndef LIBREPCB_CORE_WORKSPACE_H
#define LIBREPCB_CORE_WORKSPACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/directorylock.h"
#include "../types/version.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class Project;
class WorkspaceLibraryDb;
class WorkspaceSettings;

/*******************************************************************************
 *  Class Workspace
 ******************************************************************************/

/**
 * @brief The Workspace class represents a workspace with all its data (library,
 * projects, settings, ...)
 *
 * To access the settings of the workspace, use the method #getSettings().
 */
class Workspace final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  Workspace() = delete;
  Workspace(const Workspace& other) = delete;

  /**
   * @brief Constructor to open an existing workspace
   *
   * @param wsPath        The filepath to the workspace directory
   * @param lockCallback  A callback which gets called if the workspace
   *                      directory is locked, to decide what to do in this
   *                      case.
   *
   * @throw Exception If the workspace could not be opened, this constructor
   * throws an exception.
   */
  explicit Workspace(const FilePath& wsPath,
                     DirectoryLock::LockHandlerCallback lockCallback = nullptr);

  /**
   * The destructor
   */
  ~Workspace() noexcept;

  // Getters

  /**
   * @brief Get the filepath to the workspace directory
   */
  const FilePath& getPath() const { return mPath; }

  /**
   * @brief Get the filepath to the "projects" directory in the workspace
   */
  const FilePath& getProjectsPath() const { return mProjectsPath; }

  /**
   * @brief Get the filepath to the version directory (v#) in the workspace
   */
  const FilePath& getMetadataPath() const { return mMetadataPath; }

  /**
   * @brief Get the filepath to the "v#/libraries" directory in the workspace
   */
  const FilePath& getLibrariesPath() const { return mLibrariesPath; }

  /**
   * @brief Get the filepath to the "v#/libraries/local" directory
   */
  FilePath getLocalLibrariesPath() const {
    return mLibrariesPath.getPathTo("local");
  }

  /**
   * @brief Get the filepath to the "v#/libraries/remote" directory
   */
  FilePath getRemoteLibrariesPath() const {
    return mLibrariesPath.getPathTo("remote");
  }

  /**
   * @brief Get the workspace settings
   */
  WorkspaceSettings& getSettings() { return *mWorkspaceSettings; }

  /**
   * @brief Get the workspace settings
   */
  const WorkspaceSettings& getSettings() const { return *mWorkspaceSettings; }

  /**
   * @brief Get the workspace library database
   */
  WorkspaceLibraryDb& getLibraryDb() const { return *mLibraryDb; }

  // General Methods

  /**
   * @brief Save all (modified) settings to disk
   */
  void saveSettings();

  // Operator Overloadings
  Workspace& operator=(const Workspace& rhs) = delete;

  // Static Methods

  /**
   * @brief Check whether a filepath points to a valid workspace directory or
   * not
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
  static QList<Version> getFileFormatVersionsOfWorkspace(
      const FilePath& path) noexcept;

  /**
   * @brief getHighestFileFormatVersionOfWorkspace
   * @param path
   * @return
   */
  static tl::optional<Version> getHighestFileFormatVersionOfWorkspace(
      const FilePath& path) noexcept;

  /**
   * @brief Create a new workspace
   *
   * @param path  A path to a directory where to create the new workspace
   *
   * @throws Exception on error.
   */
  static void createNewWorkspace(const FilePath& path);

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
   * @brief Current workspace file format version (constant)
   *
   * @warning Don't change this value unless you know exactly what you're doing!
   *
   * @return File format version
   */
  static Version FILE_FORMAT_VERSION() noexcept {
    return Version::fromString("0.1");
  }

private:  // Data
  /// a FilePath object which represents the workspace directory
  FilePath mPath;

  /// the directory "projects"
  FilePath mProjectsPath;

  /// the subdirectory of the current file format version
  FilePath mMetadataPath;

  /// the directory "v#/libraries"
  FilePath mLibrariesPath;

  /// to lock the version directory (#mMetadataPath)
  DirectoryLock mLock;

  /// the WorkspaceSettings object
  QScopedPointer<WorkspaceSettings> mWorkspaceSettings;

  /// the library database
  QScopedPointer<WorkspaceLibraryDb> mLibraryDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
