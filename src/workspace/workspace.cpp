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
#include "workspacesettings.h"

using namespace library;
using namespace project;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Workspace::Workspace(const FilePath& wsPath) throw (Exception) :
    QObject(0),
    mPath(wsPath),
    mMetadataPath(wsPath.getPathTo(".metadata")),
    mProjectsPath(wsPath.getPathTo("projects")),
    mLibraryPath(wsPath.getPathTo("library")),
    mWorkspaceSettings(0), mControlPanel(0), mLibrary(0), mLibraryEditor(0),
    mProjectTreeModel(0), mRecentProjectsModel(0), mFavoriteProjectsModel(0)
{
    if ((!mPath.isExistingDir()) || (!mMetadataPath.isExistingDir()))
    {
        QMessageBox::critical(0, tr("Error"), tr("The workspace path is invalid!"));
        throw RuntimeError(QString("Invalid workspace path: \"%1\"")
                           .arg(mPath.toStr()), __FILE__, __LINE__);
    }

    if (!mProjectsPath.mkPath())
        qWarning() << "could not make path" << mProjectsPath;
    if (!mLibraryPath.mkPath())
        qWarning() << "could not make path" << mLibraryPath;


    // all OK, let's load the workspace stuff!

    try
    {
        mWorkspaceSettings = new WorkspaceSettings(*this);
        mRecentProjectsModel = new RecentProjectsModel(*this);
        mFavoriteProjectsModel = new FavoriteProjectsModel(*this);
        mProjectTreeModel = new ProjectTreeModel(*this);
        mLibrary = new Library(this);
        mControlPanel = new ControlPanel(*this, mProjectTreeModel, mRecentProjectsModel,
                                         mFavoriteProjectsModel);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(0, tr("Error"), QString(tr("Could not open the workspace!"
                                              "\n\nError message:\n%1")).arg(e.getMsg()));

        delete mControlPanel;           mControlPanel = 0;
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
    closeAllProjects(false);

    delete mControlPanel;           mControlPanel = 0;
    delete mLibraryEditor;          mLibraryEditor = 0;
    delete mLibrary;                mLibrary = 0;
    delete mProjectTreeModel;       mProjectTreeModel = 0;
    delete mFavoriteProjectsModel;  mFavoriteProjectsModel = 0;
    delete mRecentProjectsModel;    mRecentProjectsModel = 0;

    delete mWorkspaceSettings;      mWorkspaceSettings = 0;
}

/*****************************************************************************************
 *  Project Management
 ****************************************************************************************/

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
            // If a fatal error occurs while opening the project, the user gets a message
            // box with the error message. This is done directly where the error occurs.
            // Additionally, the project's constructor will throw an exception to indicate
            // that the project cannot be opened. We will catch that exception here.
            openProject = new Project(*this, filepath);
            mOpenProjects.insert(filepath.toUnique().toStr(), openProject);
            mRecentProjectsModel->setLastRecentProject(filepath);
            openProject->showSchematicEditor();
        }
        catch (Exception& e)
        {
            qWarning() << "Aborted opening the project!";
            delete openProject; openProject = 0;
        }
    }

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
            mLibraryEditor = new library_editor::LibraryEditor(this);
    }
    catch (...)
    {
        qWarning() << "Could not open the library editor!";
        return;
    }

    mLibraryEditor->show();
    mLibraryEditor->raise();
}

void Workspace::closeAllProjects(bool askForSave)
{
    QList<Project*> openProjects = mOpenProjects.values();
    foreach (Project* project, openProjects)
        closeProject(project, askForSave);
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
