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
#include "../library/library.h"
#include "../library_editor/libraryeditor.h"
#include "../project/project.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "favoriteprojectsmodel.h"
#include "controlpanel/controlpanel.h"

using namespace library;
using namespace project;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Workspace::Workspace(const QDir& workspaceDir) :
    QObject(0), mWorkspaceDir(workspaceDir),
    mMetadataDir(workspaceDir.absoluteFilePath(".metadata" % QDir::separator())),
    mWorkspaceSettings(0), mControlPanel(0), mLibrary(0), mLibraryEditor(0),
    mProjectTreeModel(0), mRecentProjectsModel(0), mFavoriteProjectsModel(0)
{
    mWorkspaceDir.makeAbsolute();
    mMetadataDir.makeAbsolute();

    if ((!mWorkspaceDir.exists()) || (!mMetadataDir.exists()))
        throw RuntimeError(QString("Invalid workspace path: \"%1\"")
                           .arg(mWorkspaceDir.absolutePath()), __FILE__, __LINE__);

    mWorkspaceSettings = new QSettings(mMetadataDir.absoluteFilePath("settings.ini"), QSettings::IniFormat);

    if ((!mWorkspaceSettings->isWritable()) || (mWorkspaceSettings->status() != QSettings::NoError))
        throw RuntimeError("Error with the workspace settings! Check file permissions!",
                           __FILE__, __LINE__);

    // all OK, let's load the workspace stuff!

    mRecentProjectsModel = new RecentProjectsModel(this);
    mFavoriteProjectsModel = new FavoriteProjectsModel(this);
    mProjectTreeModel = new ProjectTreeModel(this);
    mLibrary = new Library(this);
    mControlPanel = new ControlPanel(this, mProjectTreeModel, mRecentProjectsModel,
                                     mFavoriteProjectsModel);
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

Project* Workspace::openProject(const QString& filename)
{
    Project* openProject = getOpenProject(filename);

    if (!openProject)
    {
        openProject = new Project(this, filename);
        mOpenProjects.insert(Project::uniqueProjectFilename(filename), openProject);
        mRecentProjectsModel->setLastRecentProject(filename);
        openProject->showSchematicEditor();
    }

    return openProject;
}

bool Workspace::closeProject(const QString& filename, bool askForSave)
{
    return closeProject(getOpenProject(filename), askForSave);
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
    mOpenProjects.remove(project->getUniqueFilename());
}

Project* Workspace::getOpenProject(const QString& filename)
{
    QString uniqueFilename = Project::uniqueProjectFilename(filename);

    if (mOpenProjects.contains(uniqueFilename))
        return mOpenProjects.value(uniqueFilename);
    else
        return 0;
}

bool Workspace::isFavoriteProject(const QString& filename) const
{
    return mFavoriteProjectsModel->isFavoriteProject(filename);
}

void Workspace::addFavoriteProject(const QString& filename)
{
    mFavoriteProjectsModel->addFavoriteProject(filename);
}

void Workspace::removeFavoriteProject(const QString& filename)
{
    mFavoriteProjectsModel->removeFavoriteProject(filename);
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
    if (!mLibraryEditor)
        mLibraryEditor = new library_editor::LibraryEditor(this);

    mLibraryEditor->show();
    mLibraryEditor->raise();
}

void Workspace::closeAllProjects(bool askForSave)
{
    QList<QString> openProjectsFilenames = mOpenProjects.keys();
    foreach (QString filename, openProjectsFilenames)
    {
        closeProject(filename, askForSave);
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool Workspace::isValidWorkspaceDir(const QDir& dir)
{
    QDir workspaceDir(dir);
    QDir metadataDir(workspaceDir.absoluteFilePath(".metadata/"));

    workspaceDir.makeAbsolute();
    metadataDir.makeAbsolute();

    return ((workspaceDir.exists()) && (metadataDir.exists()));
}

void Workspace::createNewWorkspace(const QDir& dir)
{
    if (isValidWorkspaceDir(dir))
        throw RuntimeError("There is already a workspace in the selected directory!",
                           __FILE__, __LINE__);

    if (!dir.exists())
        throw RuntimeError("The selected directory does not exist!",
                           __FILE__, __LINE__);

    if (!dir.mkdir(".metadata"))
        throw RuntimeError("The .metadata directory could not be created!",
                           __FILE__, __LINE__);
}

QString Workspace::getMostRecentlyUsedWorkspacePath()
{
    QSettings clientSettings;
    return clientSettings.value("workspaces/most_recently_used").toString();
}

void Workspace::setMostRecentlyUsedWorkspacePath(const QString& path)
{
    QSettings clientSettings;
    clientSettings.setValue("workspaces/most_recently_used", path);
}

QStringList Workspace::getAllWorkspacePaths()
{
    QStringList list;
    QSettings clientSettings;
    int count = clientSettings.beginReadArray("workspaces_list");
    for (int i = 0; i < count; i++)
    {
         clientSettings.setArrayIndex(i);
         list.append(clientSettings.value("path").toString());
    }
    clientSettings.endArray();
    return list;
}

void Workspace::setAllWorkspacePaths(const QStringList& paths)
{
    QSettings clientSettings;
    clientSettings.remove("workspaces_list");
    clientSettings.beginWriteArray("workspaces_list");
    for (int i = 0; i < paths.count(); i++)
    {
         clientSettings.setArrayIndex(i);
         clientSettings.setValue("path", paths[i]);
    }
    clientSettings.endArray();
}

QString Workspace::uniqueWorkspacePath(const QString& path)
{
    QDir dir(path);
    QString uniquePath = QDir::toNativeSeparators(dir.canonicalPath());
    if (uniquePath.isEmpty())
        throw RuntimeError(QString("Invalid path: \"%1\"").arg(path), __FILE__, __LINE__);
    return uniquePath;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
