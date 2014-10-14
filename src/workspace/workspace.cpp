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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QFileDialog>
#include "workspace.h"
#include "../common/exceptions.h"
#include "../common/filepath.h"
#include "../library/library.h"
#include "../library_editor/libraryeditor.h"
#include "../project/project.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "favoriteprojectsmodel.h"
#include "controlpanel/controlpanel.h"
#include "settings/workspacesettings.h"

using namespace library;
using namespace project;

Workspace* Workspace::sInstance = 0;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Workspace::Workspace(const FilePath& wsPath) throw (Exception) :
    QObject(0),
    mPath(wsPath),
    mMetadataPath(wsPath.getPathTo(".metadata")),
    mProjectsPath(wsPath.getPathTo("projects")),
    mLibraryPath(wsPath.getPathTo("lib")),
    mWorkspaceSettings(0), mControlPanel(0), mLibrary(0), mLibraryEditor(0),
    mProjectTreeModel(0), mRecentProjectsModel(0), mFavoriteProjectsModel(0)
{
    if (sInstance != 0) throw LogicError(__FILE__, __LINE__); // should never happen...
    sInstance = this;

    try
    {
        // check the workspace path
        if ((!mPath.isExistingDir()) || (!mMetadataPath.isExistingDir()))
        {
            throw RuntimeError(__FILE__, __LINE__, mPath.toStr(),
                QString(tr("Invalid workspace path: \"%1\"")).arg(mPath.toNative()));
        }

        if (!mProjectsPath.mkPath())
            qWarning() << "could not make path" << mProjectsPath;
        if (!mLibraryPath.mkPath())
            qWarning() << "could not make path" << mLibraryPath;

        // all OK, let's load the workspace stuff!

        mWorkspaceSettings = new WorkspaceSettings();
        mRecentProjectsModel = new RecentProjectsModel();
        mFavoriteProjectsModel = new FavoriteProjectsModel();
        mProjectTreeModel = new ProjectTreeModel();
        mLibrary = new Library();
        mControlPanel = new ControlPanel(mProjectTreeModel, mRecentProjectsModel,
                                         mFavoriteProjectsModel);
    }
    catch (Exception& e)
    {
        // free allocated memory and rethrow the exception
        delete mControlPanel;           mControlPanel = 0;
        delete mLibrary;                mLibrary = 0;
        delete mProjectTreeModel;       mProjectTreeModel = 0;
        delete mFavoriteProjectsModel;  mFavoriteProjectsModel = 0;
        delete mRecentProjectsModel;    mRecentProjectsModel = 0;
        delete mWorkspaceSettings;      mWorkspaceSettings = 0;

        sInstance = 0;
        throw;
    }
}

Workspace::~Workspace()
{
    closeAllProjects(false);

    delete mControlPanel;           mControlPanel = 0;
    delete mLibraryEditor;          mLibraryEditor = 0;
    delete mLibrary;                mLibrary = 0;
    delete mProjectTreeModel;       mProjectTreeModel = 0;
    delete mFavoriteProjectsModel;  mFavoriteProjectsModel = 0;
    delete mRecentProjectsModel;    mRecentProjectsModel = 0;
    delete mWorkspaceSettings;      mWorkspaceSettings = 0;

    sInstance = 0;
}

/*****************************************************************************************
 *  Project Management
 ****************************************************************************************/

Project* Workspace::createProject(const FilePath& filepath) noexcept
{
    Project* project = 0;

    try
    {
        project = Project::create(filepath);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(mControlPanel, tr("Cannot create the project!"), e.getUserMsg());
        return 0;
    }

    // project successfully created and opened!
    mOpenProjects.insert(filepath.toUnique().toStr(), project);
    mRecentProjectsModel->setLastRecentProject(filepath);

    project->showSchematicEditor();

    return project;
}

Project* Workspace::openProject(const FilePath& filepath) noexcept
{
    // Check if the filepath is an existing file
    if (!filepath.isExistingFile())
    {
        QMessageBox::critical(0, tr("Invalid filename"), QString(
            tr("The project filename is not valid: \"%1\"")).arg(filepath.toNative()));
        return 0;
    }

    Project* openProject = getOpenProject(filepath);

    if (!openProject)
    {
        try
        {
            // If a fatal error occurs while opening the project, the project's
            // constructor will throw an exception. We will catch that exception here and
            // show a message box to print the error message to the monitor. Only
            // exceptions of type "UserCanceled" are ignored.
            openProject = new Project(filepath);
        }
        catch (UserCanceled& e)
        {
            // the user has canceled opening the project, so we ignore this exception...
            qDebug() << "Aborted opening the project!";
            return 0;
        }
        catch (Exception& e)
        {
            // opening the project was interrupted by an exception!
            qDebug() << "Aborted opening the project!";
            QMessageBox::critical(mControlPanel, tr("Cannot open the project!"),
                                  e.getUserMsg());
            return 0;
        }

        // project successfully opened!
        mOpenProjects.insert(filepath.toUnique().toStr(), openProject);
        mRecentProjectsModel->setLastRecentProject(filepath);
    }

    openProject->showSchematicEditor();

    return openProject;
}

bool Workspace::closeProject(const FilePath& filepath, bool askForSave)
{
    return closeProject(getOpenProject(filepath), askForSave);
}

bool Workspace::closeProject(Project* project, bool askForSave)
{
    if (!project)
        return true;

    bool success = true;

    if (askForSave)
        success = project->close();

    if (success)
        delete project;

    return success;
}

bool Workspace::closeAllProjects(bool askForSave)
{
    bool success = true;
    foreach (Project* project, mOpenProjects)
    {
        if (!closeProject(project, askForSave))
            success = false;
    }
    return success;
}

void Workspace::unregisterOpenProject(Project* project)
{
    mOpenProjects.remove(project->getFilepath().toUnique().toStr());
}

Project* Workspace::getOpenProject(const FilePath& filepath)
{
    if (mOpenProjects.contains(filepath.toUnique().toStr()))
        return mOpenProjects.value(filepath.toUnique().toStr());
    else
        return 0;
}

bool Workspace::isFavoriteProject(const FilePath& filepath) const
{
    return mFavoriteProjectsModel->isFavoriteProject(filepath);
}

void Workspace::addFavoriteProject(const FilePath& filepath)
{
    mFavoriteProjectsModel->addFavoriteProject(filepath);
}

void Workspace::removeFavoriteProject(const FilePath& filepath)
{
    mFavoriteProjectsModel->removeFavoriteProject(filepath);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void Workspace::showControlPanel() const
{
    mControlPanel->show();
    mControlPanel->raise();
}

void Workspace::openLibraryEditor()
{
    try
    {
        if (!mLibraryEditor)
            mLibraryEditor = new library_editor::LibraryEditor();
    }
    catch (...)
    {
        qWarning() << "Could not open the library editor!";
        return;
    }

    mLibraryEditor->show();
    mLibraryEditor->raise();
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool Workspace::isValidWorkspacePath(const FilePath& path)
{
    if (!path.isExistingDir())
        return false;
    if (!path.getPathTo(".metadata").isExistingDir())
        return false;

    return true;
}

bool Workspace::createNewWorkspace(const FilePath& path)
{
    if (isValidWorkspacePath(path))
        return true;

    // create directory ".metadata" (and all needed parent directories)
    return path.getPathTo(".metadata").mkPath();
}

FilePath Workspace::getMostRecentlyUsedWorkspacePath()
{
    QSettings clientSettings;
    return FilePath(clientSettings.value("workspaces/most_recently_used").toString());
}

void Workspace::setMostRecentlyUsedWorkspacePath(const FilePath& path)
{
    QSettings clientSettings;
    clientSettings.setValue("workspaces/most_recently_used", path.toNative());
}

FilePath Workspace::chooseWorkspacePath()
{
    FilePath path(QFileDialog::getExistingDirectory(0, tr("Select Workspace Path")));

    if (!path.isValid())
        return FilePath();

    if (!isValidWorkspacePath(path))
    {
        int answer = QMessageBox::question(0, tr("Create new workspace?"),
                        tr("The speciefied workspace does not exist. "
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
