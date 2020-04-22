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

#include "favoriteprojectsmodel.h"
#include "library/workspacelibrarydb.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "settings/workspacesettings.h"

#include <librepcb/common/application.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/versionfile.h>
#include <librepcb/library/library.h>
#include <librepcb/libraryeditor/libraryeditor.h>
#include <librepcb/project/project.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

using namespace library;
using namespace project;

namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Workspace::Workspace(const FilePath& wsPath)
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
        QString(tr("Invalid workspace path: \"%1\"")).arg(mPath.toNative()));
  }
  FilePath    versionFp = mPath.getPathTo(".librepcb-workspace");
  QByteArray  versionRaw(FileUtils::readFile(versionFp));  // can throw
  VersionFile wsVersionFile =
      VersionFile::fromByteArray(versionRaw);  // can throw
  if (wsVersionFile.getVersion() != FILE_FORMAT_VERSION()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString(tr("The workspace version %1 is not compatible "
                                  "with this application version."))
                           .arg(wsVersionFile.getVersion().toStr()));
  }

  // create directories which do not exist already
  FileUtils::makePath(mProjectsPath);   // can throw
  FileUtils::makePath(mMetadataPath);   // can throw
  FileUtils::makePath(mLibrariesPath);  // can throw

  // Check if the workspace is locked (already open or application crashed).
  switch (mLock.getStatus())  // can throw
  {
    case DirectoryLock::LockStatus::Unlocked: {
      // nothing to do here (the workspace will be locked later)
      break;
    }
    case DirectoryLock::LockStatus::Locked: {
      // the workspace is locked by another application instance
      throw RuntimeError(__FILE__, __LINE__,
                         tr("The workspace is already "
                            "opened by another application instance or user!"));
    }
    case DirectoryLock::LockStatus::StaleLock: {
      // ignore stale lock as there is nothing to restore
      qWarning() << "There was a stale lock on the workspace:" << mPath;
      break;
    }
    default:
      Q_ASSERT(false);
      throw LogicError(__FILE__, __LINE__);
  }

  // the workspace can be opened by this application, so we will lock it
  mLock.lock();  // can throw

  // all OK, let's load the workspace stuff!

  // load workspace settings
  mWorkspaceSettings.reset(
      new WorkspaceSettings(mMetadataPath.getPathTo("settings.lp"), this));

  // load library database
  mLibraryDb.reset(new WorkspaceLibraryDb(*this));  // can throw

  // load project models
  mRecentProjectsModel.reset(new RecentProjectsModel(*this));
  mFavoriteProjectsModel.reset(new FavoriteProjectsModel(*this));
  mProjectTreeModel.reset(new ProjectTreeModel(*this));
}

Workspace::~Workspace() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

ProjectTreeModel& Workspace::getProjectTreeModel() const noexcept {
  return *mProjectTreeModel;
}

RecentProjectsModel& Workspace::getRecentProjectsModel() const noexcept {
  return *mRecentProjectsModel;
}

FavoriteProjectsModel& Workspace::getFavoriteProjectsModel() const noexcept {
  return *mFavoriteProjectsModel;
}

/*******************************************************************************
 *  Project Management
 ******************************************************************************/

void Workspace::setLastRecentlyUsedProject(const FilePath& filepath) noexcept {
  mRecentProjectsModel->setLastRecentProject(filepath);
}

bool Workspace::isFavoriteProject(const FilePath& filepath) const noexcept {
  return mFavoriteProjectsModel->isFavoriteProject(filepath);
}

void Workspace::addFavoriteProject(const FilePath& filepath) noexcept {
  mFavoriteProjectsModel->addFavoriteProject(filepath);
}

void Workspace::removeFavoriteProject(const FilePath& filepath) noexcept {
  mFavoriteProjectsModel->removeFavoriteProject(filepath);
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
    QDir        dir(path.toStr());
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

}  // namespace workspace
}  // namespace librepcb
