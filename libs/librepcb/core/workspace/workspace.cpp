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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspace.h"

#include "../application.h"
#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../fileio/fileutils.h"
#include "../fileio/versionfile.h"
#include "../library/library.h"
#include "../project/project.h"
#include "workspacelibrarydb.h"
#include "workspacesettings.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Workspace::Workspace(const FilePath& wsPath,
                     DirectoryLock::LockHandlerCallback lockCallback)
  : QObject(nullptr),
    mPath(wsPath),
    mProjectsPath(mPath.getPathTo("projects")),
    mMetadataPath(mPath.getPathTo("v" % qApp->getFileFormatVersion().toStr())),
    mLibrariesPath(mMetadataPath.getPathTo("libraries")),
    mLock(mMetadataPath) {
  // check if the workspace is valid
  if (!isValidWorkspacePath(mPath)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Invalid workspace path: \"%1\"").arg(mPath.toNative()));
  }
  FilePath versionFp = mPath.getPathTo(".librepcb-workspace");
  QByteArray versionRaw(FileUtils::readFile(versionFp));  // can throw
  VersionFile wsVersionFile =
      VersionFile::fromByteArray(versionRaw);  // can throw
  if (wsVersionFile.getVersion() != FILE_FORMAT_VERSION()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The workspace version %1 is not compatible "
                          "with this application version.")
                           .arg(wsVersionFile.getVersion().toStr()));
  }

  // create directories which do not exist already
  FileUtils::makePath(mProjectsPath);  // can throw
  FileUtils::makePath(mMetadataPath);  // can throw
  FileUtils::makePath(mLibrariesPath);  // can throw

  // the workspace can be opened by this application, so we will lock it
  mLock.tryLock(lockCallback);  // can throw

  // all OK, let's load the workspace stuff!

  // load workspace settings
  mWorkspaceSettings.reset(
      new WorkspaceSettings(mMetadataPath.getPathTo("settings.lp"),
                            qApp->getFileFormatVersion(), this));

  // load library database
  mLibraryDb.reset(new WorkspaceLibraryDb(*this));  // can throw
}

Workspace::~Workspace() noexcept {
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool Workspace::isValidWorkspacePath(const FilePath& path) noexcept {
  return path.getPathTo(".librepcb-workspace").isExistingFile();
}

QList<Version> Workspace::getFileFormatVersionsOfWorkspace(
    const FilePath& path) noexcept {
  QList<Version> list;
  if (isValidWorkspacePath(path)) {
    QDir dir(path.toStr());
    QStringList subdirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    foreach (const QString& subdir, subdirs) {
      if (subdir.startsWith('v')) {
        tl::optional<Version> version =
            Version::tryFromString(subdir.mid(1, -1));
        if (version) {
          list.append(*version);
        }
      }
    }
    std::sort(list.begin(), list.end());
  }
  return list;
}

tl::optional<Version> Workspace::getHighestFileFormatVersionOfWorkspace(
    const FilePath& path) noexcept {
  QList<Version> versions = getFileFormatVersionsOfWorkspace(path);
  if (versions.count() > 0) {
    return versions.last();
  } else {
    return tl::nullopt;
  }
}

void Workspace::createNewWorkspace(const FilePath& path) {
  FileUtils::writeFile(
      path.getPathTo(".librepcb-workspace"),
      VersionFile(FILE_FORMAT_VERSION()).toByteArray());  // can throw
}

FilePath Workspace::getMostRecentlyUsedWorkspacePath() noexcept {
  QSettings clientSettings;
  return FilePath(
      clientSettings.value("workspaces/most_recently_used").toString());
}

void Workspace::setMostRecentlyUsedWorkspacePath(
    const FilePath& path) noexcept {
  QSettings clientSettings;
  clientSettings.setValue("workspaces/most_recently_used", path.toNative());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
