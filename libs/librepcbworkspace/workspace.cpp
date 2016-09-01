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
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/fileio/fileutils.h>
#include <librepcbcommon/application.h>
#include <librepcblibraryeditor/libraryeditor.h>
#include <librepcbproject/project.h>
#include "library/workspacelibrary.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "favoriteprojectsmodel.h"
#include "settings/workspacesettings.h"
#include <librepcbcommon/schematiclayer.h>

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

Workspace::Workspace(const FilePath& wsPath) throw (Exception) :
    QObject(0),
    mPath(wsPath), mLock(wsPath),
    mMetadataPath(wsPath.getPathTo(QString(".metadata/v%1").arg(qApp->getFileFormatVersion().getNumbers().value(0)))),
    mProjectsPath(wsPath.getPathTo("projects")),
    mLibraryPath(wsPath.getPathTo("library")),
    mWorkspaceSettings(0), mLibrary(0), mProjectTreeModel(0), mRecentProjectsModel(0),
    mFavoriteProjectsModel(0)
{
    try
    {
        // check the workspace path
        if ((!mPath.isExistingDir()) || (!mMetadataPath.isExistingDir()))
        {
            throw RuntimeError(__FILE__, __LINE__, mPath.toStr(),
                QString(tr("Invalid workspace path: \"%1\"")).arg(mPath.toNative()));
        }

        // Check if the workspace is locked (already open or application was crashed).
        switch (mLock.getStatus()) // can throw
        {
            case DirectoryLock::LockStatus::Unlocked: {
                // nothing to do here (the workspace will be locked later)
                break;
            }
            case DirectoryLock::LockStatus::Locked: {
                // the workspace is locked by another application instance
                throw RuntimeError(__FILE__, __LINE__, QString(), tr("The workspace is already "
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

        // create directories (if not already exist)
        FileUtils::makePath(mProjectsPath); // can throw
        FileUtils::makePath(mLibraryPath); // can throw

        // all OK, let's load the workspace stuff!

        mWorkspaceSettings = new WorkspaceSettings(*this);
        mRecentProjectsModel = new RecentProjectsModel(*this);
        mFavoriteProjectsModel = new FavoriteProjectsModel(*this);
        mProjectTreeModel = new ProjectTreeModel(*this);
        mLibrary = new WorkspaceLibrary(*this);
    }
    catch (Exception& e)
    {
        // free allocated memory and rethrow the exception
        delete mLibrary;                mLibrary = 0;
        delete mProjectTreeModel;       mProjectTreeModel = 0;
        delete mFavoriteProjectsModel;  mFavoriteProjectsModel = 0;
        delete mRecentProjectsModel;    mRecentProjectsModel = 0;
        delete mWorkspaceSettings;      mWorkspaceSettings = 0;
        throw;
    }
}

Workspace::~Workspace()
{
    delete mLibrary;                mLibrary = 0;
    delete mProjectTreeModel;       mProjectTreeModel = 0;
    delete mFavoriteProjectsModel;  mFavoriteProjectsModel = 0;
    delete mRecentProjectsModel;    mRecentProjectsModel = 0;
    delete mWorkspaceSettings;      mWorkspaceSettings = 0;
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
    if (!path.isExistingDir())
        return false;
    if (!path.getPathTo(".metadata").isExistingDir())
        return false;

    return true;
}

bool Workspace::createNewWorkspace(const FilePath& path) noexcept
{
    if (isValidWorkspacePath(path)) {
        return true;
    }

    // create directory ".metadata/v#/" (and all needed parent directories)
    try {
        FilePath versionDir = path.getPathTo(QString(".metadata/v%1").arg(
            qApp->getFileFormatVersion().getNumbers().value(0)));
        FileUtils::makePath(versionDir); // can throw
        return true;
    } catch (...) {
        return false;
    }
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

    if (!path.isValid())
        return FilePath();

    if (!isValidWorkspacePath(path))
    {
        int answer = QMessageBox::question(0, tr("Create new workspace?"),
                        tr("The specified workspace does not exist. "
                           "Do you want to create a new workspace?"));

        if (answer == QMessageBox::Yes)
        {
            if (!createNewWorkspace(path))
            {
                QMessageBox::critical(0, tr("Error"), tr("Could not create the workspace!"));
                return FilePath();
            }
        }
        else
            return FilePath();
    }

    return path;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
