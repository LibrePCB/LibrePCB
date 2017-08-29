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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QFileDialog>
#include "workspace.h"
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/smartversionfile.h>
#include <librepcb/common/application.h>
#include <librepcb/libraryeditor/libraryeditor.h>
#include <librepcb/project/project.h>
#include "library/workspacelibrarydb.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "favoriteprojectsmodel.h"
#include "settings/workspacesettings.h"
#include <librepcb/library/library.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

using namespace library;
using namespace project;

namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Workspace::Workspace(const FilePath& wsPath) :
    QObject(nullptr),
    mPath(wsPath),
    mProjectsPath(mPath.getPathTo("projects")),
    mVersionPath(mPath.getPathTo("v" % qApp->getFileFormatVersion().toStr())),
    mMetadataPath(mVersionPath.getPathTo("metadata")),
    mLibrariesPath(mVersionPath.getPathTo("libraries")),
    mLock(mVersionPath)
{
    // check if the workspace is valid
    if (!isValidWorkspacePath(mPath)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid workspace path: \"%1\"")).arg(mPath.toNative()));
    }
    SmartVersionFile wsVersionFile(mPath.getPathTo(".librepcb-workspace"), false, true); // can throw
    if (wsVersionFile.getVersion() != FILE_FORMAT_VERSION()) {
        throw RuntimeError(__FILE__, __LINE__, QString(tr(
            "The workspace version %1 is not compatible with this application version."))
            .arg(wsVersionFile.getVersion().toStr()));
    }

    // create directories which do not exist already
    FileUtils::makePath(mProjectsPath); // can throw
    FileUtils::makePath(mMetadataPath); // can throw
    FileUtils::makePath(mLibrariesPath); // can throw

    // Check if the workspace is locked (already open or application was crashed).
    switch (mLock.getStatus()) // can throw
    {
        case DirectoryLock::LockStatus::Unlocked: {
            // nothing to do here (the workspace will be locked later)
            break;
        }
        case DirectoryLock::LockStatus::Locked: {
            // the workspace is locked by another application instance
            throw RuntimeError(__FILE__, __LINE__, tr("The workspace is already "
                               "opened by another application instance or user!"));
        }
        case DirectoryLock::LockStatus::StaleLock:{
            // ignore stale lock as there is nothing to restore
            qWarning() << "There was a stale lock on the workspace:" << mPath;
            break;
        }
        default: Q_ASSERT(false); throw LogicError(__FILE__, __LINE__);
    }

    // the workspace can be opened by this application, so we will lock it
    mLock.lock(); // can throw

    // all OK, let's load the workspace stuff!

    // load workspace settings
    mWorkspaceSettings.reset(new WorkspaceSettings(*this));

    // load local libraries
    FilePath localLibsDirPath = mLibrariesPath.getPathTo("local");
    QDir localLibsDir(localLibsDirPath.toStr());
    foreach (const QString& dir, localLibsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        FilePath libDirPath = localLibsDirPath.getPathTo(dir);
        if (Library::isValidElementDirectory<Library>(libDirPath)) {
            qDebug() << "Load local workspace library:" << dir;
            try {
                addLocalLibrary(dir); // can throw
            } catch (Exception& e) {
                // @todo do not show a message box here, better use something like a
                // getLastError() method which is used by the ControlPanel to show errors
                QMessageBox::critical(nullptr, tr("Error"), QString(tr(
                    "Could not open local library %1: %2")).arg(dir, e.getMsg()));
            }
        } else {
            qWarning() << "Directory is not a valid libary:" << libDirPath.toNative();
        }
    }

    // load remote libraries
    FilePath remoteLibsDirPath = mLibrariesPath.getPathTo("remote");
    QDir remoteLibsDir(remoteLibsDirPath.toStr());
    foreach (const QString& dir, remoteLibsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        FilePath libDirPath = remoteLibsDirPath.getPathTo(dir);
        if (Library::isValidElementDirectory<Library>(libDirPath)) {
            qDebug() << "Load remote workspace library:" << dir;
            try {
                addRemoteLibrary(dir); // can throw
            } catch (Exception& e) {
                // @todo do not show a message box here, better use something like a
                // getLastError() method which is used by the ControlPanel to show errors
                QMessageBox::critical(nullptr, tr("Error"), QString(tr(
                    "Could not open remote library %1: %2")).arg(dir, e.getMsg()));
            }
        } else {
            qWarning() << "Directory is not a valid libary:" << libDirPath.toNative();
        }
    }

    // load library database
    mLibraryDb.reset(new WorkspaceLibraryDb(*this)); // can throw
    connect(this, &Workspace::libraryAdded,
            mLibraryDb.data(), &WorkspaceLibraryDb::startLibraryRescan);
    connect(this, &Workspace::libraryRemoved,
            mLibraryDb.data(), &WorkspaceLibraryDb::startLibraryRescan);

    // load project models
    mRecentProjectsModel.reset(new RecentProjectsModel(*this));
    mFavoriteProjectsModel.reset(new FavoriteProjectsModel(*this));
    mProjectTreeModel.reset(new ProjectTreeModel(*this));
}

Workspace::~Workspace() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QAbstractItemModel& Workspace::getProjectTreeModel() const noexcept
{
    return *mProjectTreeModel;
}

QAbstractItemModel& Workspace::getRecentProjectsModel() const noexcept
{
    return *mRecentProjectsModel;
}

QAbstractItemModel& Workspace::getFavoriteProjectsModel() const noexcept
{
    return *mFavoriteProjectsModel;
}

/*****************************************************************************************
 *  Library Management
 ****************************************************************************************/

Version Workspace::getVersionOfLibrary(const Uuid& uuid, bool local, bool remote) const noexcept
{
    Version version;
    if (local) {
        foreach (const auto& lib, mLocalLibraries) { Q_ASSERT(lib);
            if ((lib->getUuid() == uuid) && ((!version.isValid()) || (version < lib->getVersion()))) {
                version = lib->getVersion();
            }
        }
    }
    if (remote) {
        foreach (const auto& lib, mRemoteLibraries) { Q_ASSERT(lib);
            if ((lib->getUuid() == uuid) && ((!version.isValid()) || (version < lib->getVersion()))) {
                version = lib->getVersion();
            }
        }
    }
    return version;
}

void Workspace::addLocalLibrary(const QString& libDirName)
{
    if (!mLocalLibraries.contains(libDirName)) {
        FilePath libDirPath = mLibrariesPath.getPathTo("local").getPathTo(libDirName);
        QSharedPointer<Library> library(new Library(libDirPath, false)); // can throw
        mLocalLibraries.insert(libDirName, library);
        emit libraryAdded(libDirPath);
    }
}

void Workspace::addRemoteLibrary(const QString& libDirName)
{
    if (!mRemoteLibraries.contains(libDirName)) {
        // remote libraries are always opened read-only!
        FilePath libDirPath = mLibrariesPath.getPathTo("remote").getPathTo(libDirName);
        QSharedPointer<Library> library(new Library(libDirPath, true)); // can throw
        mRemoteLibraries.insert(libDirName, library);
        emit libraryAdded(libDirPath);
    }
}

void Workspace::removeLocalLibrary(const QString& libDirName, bool rmDir)
{
    Library* library = mLocalLibraries.value(libDirName).data();
    if (library) {
        FilePath libDirPath = library->getFilePath();
        mLocalLibraries.remove(libDirName);
        emit libraryRemoved(libDirPath);
        if (rmDir)  FileUtils::removeDirRecursively(libDirPath); // can throw
    }
}

void Workspace::removeRemoteLibrary(const QString& libDirName, bool rmDir)
{
    Library* library = mRemoteLibraries.value(libDirName).data();
    if (library) {
        FilePath libDirPath = library->getFilePath();
        mRemoteLibraries.remove(libDirName);
        emit libraryRemoved(libDirPath);
        if (rmDir) FileUtils::removeDirRecursively(libDirPath); // can throw
    }
}

/*****************************************************************************************
 *  Project Management
 ****************************************************************************************/

void Workspace::setLastRecentlyUsedProject(const FilePath& filepath) noexcept
{
    mRecentProjectsModel->setLastRecentProject(filepath);
}

bool Workspace::isFavoriteProject(const FilePath& filepath) const noexcept
{
    return mFavoriteProjectsModel->isFavoriteProject(filepath);
}

void Workspace::addFavoriteProject(const FilePath& filepath) noexcept
{
    mFavoriteProjectsModel->addFavoriteProject(filepath);
}

void Workspace::removeFavoriteProject(const FilePath& filepath) noexcept
{
    mFavoriteProjectsModel->removeFavoriteProject(filepath);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool Workspace::isValidWorkspacePath(const FilePath& path) noexcept
{
    return path.getPathTo(".librepcb-workspace").isExistingFile();
}

QList<Version> Workspace::getFileFormatVersionsOfWorkspace(const FilePath& path) noexcept
{
    QList<Version> list;
    if (isValidWorkspacePath(path)) {
        QDir dir(path.toStr());
        QStringList subdirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        foreach (const QString& subdir, subdirs) {
            if (subdir.startsWith('v')) {
                Version version(subdir.mid(1, -1));
                if (version.isValid()) {
                    list.append(version);
                }
            }
        }
        qSort(list);
    }
    return list;
}

Version Workspace::getHighestFileFormatVersionOfWorkspace(const FilePath& path) noexcept
{
    QList<Version> versions = getFileFormatVersionsOfWorkspace(path);
    if (versions.count() > 0) {
        return versions.last();
    } else {
        return Version();
    }
}

void Workspace::createNewWorkspace(const FilePath& path)
{
    FilePath versionFilePath(path.getPathTo(".librepcb-workspace"));
    QScopedPointer<SmartVersionFile> versionFile(
        SmartVersionFile::create(versionFilePath, FILE_FORMAT_VERSION())); // can throw
    versionFile->save(true); // can throw
}

FilePath Workspace::getMostRecentlyUsedWorkspacePath() noexcept
{
    QSettings clientSettings;
    return FilePath(clientSettings.value("workspaces/most_recently_used").toString());
}

void Workspace::setMostRecentlyUsedWorkspacePath(const FilePath& path) noexcept
{
    QSettings clientSettings;
    clientSettings.setValue("workspaces/most_recently_used", path.toNative());
}

FilePath Workspace::chooseWorkspacePath() noexcept
{
    FilePath path(QFileDialog::getExistingDirectory(0, tr("Select Workspace Path")));

    if ((path.isValid()) && (!isValidWorkspacePath(path))) {
        int answer = QMessageBox::question(0, tr("Create new workspace?"),
                        tr("The specified workspace does not exist. "
                           "Do you want to create a new workspace?"));

        if (answer == QMessageBox::Yes) {
            try {
                createNewWorkspace(path); // can throw
            } catch (const Exception& e) {
                QMessageBox::critical(0, tr("Error"), tr("Could not create the workspace!"));
                return FilePath();
            }
        }
        else {
            return FilePath();
        }
    }

    return path;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
