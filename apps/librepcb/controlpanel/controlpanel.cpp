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
#include <QtWidgets>
#include <QFileDialog>
#include <QQuickView>
#include <QQmlContext>
#include "controlpanel.h"
#include "ui_controlpanel.h"
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/project/project.h>
#include <librepcb/workspace/projecttreemodel.h>
#include <librepcb/workspace/recentprojectsmodel.h>
#include <librepcb/workspace/favoriteprojectsmodel.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/librarymanager/librarymanager.h>
#include <librepcb/library/library.h>
#include <librepcb/libraryeditor/libraryeditor.h>
#include <librepcb/projecteditor/projecteditor.h>
#include <librepcb/projecteditor/newprojectwizard/newprojectwizard.h>
#include <librepcb/common/application.h>
#include <librepcb/common/fileio/fileutils.h>
#include "../markdown/markdownconverter.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace application {

using namespace project;
using namespace project::editor;
using namespace library::manager;
using namespace workspace;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ControlPanel::ControlPanel(Workspace& workspace) :
    QMainWindow(nullptr), mWorkspace(workspace), mUi(new Ui::ControlPanel),
    mLibraryManager(new LibraryManager(mWorkspace, this))
{
    mUi->setupUi(this);

    setWindowTitle(QString(tr("Control Panel - LibrePCB %1"))
                   .arg(qApp->getAppVersion().toPrettyStr(2)));

    // show workspace path in status bar
    QString wsPath = mWorkspace.getPath().toNative();
    QLabel* statusBarLabel = new QLabel(QString(tr("Workspace: %1")).arg(wsPath));
    mUi->statusBar->addWidget(statusBarLabel, 1);

    // initialize status bar
    mUi->statusBar->setFields(StatusBar::ProgressBar);
    mUi->statusBar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
    connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanStarted,
            mUi->statusBar, &StatusBar::showProgressBar, Qt::QueuedConnection);
    connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanSucceeded,
            mUi->statusBar, &StatusBar::hideProgressBar, Qt::QueuedConnection);
    connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanProgressUpdate,
            mUi->statusBar, &StatusBar::setProgressBarPercent, Qt::QueuedConnection);

    // decive if we have to show the warning about a newer workspace file format version
    Version actualVersion = qApp->getFileFormatVersion();
    Version highestVersion = Workspace::getHighestFileFormatVersionOfWorkspace(workspace.getPath());
    mUi->lblWarnForNewerAppVersions->setVisible(highestVersion > actualVersion);

    // decide if we have to show the warning about missing workspace libraries
    if (mWorkspace.getLocalLibraries().isEmpty() && mWorkspace.getRemoteLibraries().isEmpty()) {
        mUi->lblWarnForNoLibraries->setVisible(true);
        connect(mUi->lblWarnForNoLibraries, &QLabel::linkActivated,
                this, &ControlPanel::on_actionOpen_Library_Manager_triggered);
        connect(&mWorkspace, &Workspace::libraryAdded,
                mUi->lblWarnForNoLibraries, &QLabel::hide);
    } else {
        mUi->lblWarnForNoLibraries->setVisible(false);
    }

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionQuit, &QAction::triggered,
            this, &ControlPanel::close);
    //connect(mUi->actionOpen_Library_Editor, &QAction::triggered,
    //        &Workspace::instance(), &Workspace::openLibraryEditor);
    connect(mUi->actionAbout_Qt, &QAction::triggered,
            qApp, &QApplication::aboutQt);
    connect(mUi->actionWorkspace_Settings, &QAction::triggered,
            &mWorkspace.getSettings(), &WorkspaceSettings::showSettingsDialog);
    connect(mLibraryManager.data(), &LibraryManager::openLibraryEditorTriggered,
            this, &ControlPanel::openLibraryEditor);

    // build projects file tree
    mUi->projectTreeView->setModel(&mWorkspace.getProjectTreeModel());
    mUi->projectTreeView->setRootIndex(mWorkspace.getProjectTreeModel().index(mWorkspace.getProjectsPath().toStr()));
    for (int i = 1; i < mUi->projectTreeView->header()->count(); ++i) {
        mUi->projectTreeView->hideColumn(i);
    }

    // load recent and favorite poject models
    mUi->recentProjectsListView->setModel(&mWorkspace.getRecentProjectsModel());
    mUi->favoriteProjectsListView->setModel(&mWorkspace.getFavoriteProjectsModel());

    loadSettings();

    // parse command line arguments and open all project files
    foreach (const QString& arg, qApp->arguments())
    {
        FilePath filepath(arg);
        if ((filepath.isExistingFile()) && (filepath.getSuffix() == "lpp"))
            openProject(filepath);
    }

    // start scanning the workspace library (asynchronously)
    mWorkspace.getLibraryDb().startLibraryRescan();
}

ControlPanel::~ControlPanel()
{
    closeAllProjects(false);
    closeAllLibraryEditors(false);
    mLibraryManager.reset();
    mUi.reset();
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    // close all projects, unsaved projects will ask for saving
    if (!closeAllProjects(true)) {
        event->ignore();
        return; // do NOT close the application, there are still open projects!
    }

    // close all library editors, unsaved libraries will ask for saving
    if (!closeAllLibraryEditors(true)) {
        event->ignore();
        return; // do NOT close the application, there are still open library editors!
    }

    saveSettings();

    QMainWindow::closeEvent(event);

    // if the control panel is closed, we will quit the whole application
    QApplication::quit();
}

void ControlPanel::showControlPanel() noexcept
{
    show();
    raise();
    activateWindow();
}

/*****************************************************************************************
 *  General private methods
 ****************************************************************************************/

void ControlPanel::saveSettings()
{
    QSettings clientSettings;
    clientSettings.beginGroup("controlpanel");

    // main window
    clientSettings.setValue("window_geometry", saveGeometry());
    clientSettings.setValue("window_state", saveState());
    clientSettings.setValue("splitter_h_state", mUi->splitter_h->saveState());
    clientSettings.setValue("splitter_v_state", mUi->splitter_v->saveState());

    // projects treeview (expanded items)
    if (ProjectTreeModel* model = dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model())) {
        QStringList list;
        foreach (QModelIndex index, model->getPersistentIndexList()) {
            if (mUi->projectTreeView->isExpanded(index)) {
                list.append(FilePath(model->filePath(index)).toRelative(mWorkspace.getPath()));
            }
        }
        clientSettings.setValue("expanded_projecttreeview_items", QVariant::fromValue(list));
    }

    clientSettings.endGroup();
}

void ControlPanel::loadSettings()
{
    QSettings clientSettings;
    clientSettings.beginGroup("controlpanel");

    // main window
    restoreGeometry(clientSettings.value("window_geometry").toByteArray());
    restoreState(clientSettings.value("window_state").toByteArray());
    mUi->splitter_h->restoreState(clientSettings.value("splitter_h_state").toByteArray());
    mUi->splitter_v->restoreState(clientSettings.value("splitter_v_state").toByteArray());

    // projects treeview (expanded items)
    if (ProjectTreeModel* model = dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model())) {
        QStringList list = clientSettings.value("expanded_projecttreeview_items").toStringList();
        foreach (QString item, list) {
            FilePath filepath = FilePath::fromRelative(mWorkspace.getPath(), item);
            QModelIndex index = model->index(filepath.toStr());
            mUi->projectTreeView->setExpanded(index, true);
        }
    }

    clientSettings.endGroup();
}

void ControlPanel::showProjectReadmeInBrowser(const FilePath& projectFilePath) noexcept
{
    if (projectFilePath.isValid()) {
        FilePath readmeFilePath = projectFilePath.getPathTo("README.md");
        mUi->textBrowser->setSearchPaths(QStringList(projectFilePath.toStr()));
        mUi->textBrowser->setHtml(MarkdownConverter::convertMarkdownToHtml(readmeFilePath));
    } else {
        mUi->textBrowser->clear();
    }
}

/*****************************************************************************************
 *  Project Management
 ****************************************************************************************/

ProjectEditor* ControlPanel::newProject(const FilePath& parentDir) noexcept
{
    NewProjectWizard wizard(mWorkspace, this);
    wizard.setLocation(parentDir);
    if (wizard.exec() == QWizard::Accepted) {
        try {
            QScopedPointer<Project> project(wizard.createProject()); // can throw
            return openProject(*project.take());
        } catch (Exception& e) {
            QMessageBox::critical(this, tr("Could not create project"), e.getMsg());
        }
    }
    return nullptr;
}

ProjectEditor* ControlPanel::openProject(Project& project) noexcept
{
    try
    {
        ProjectEditor* editor = getOpenProject(project.getFilepath());
        if (!editor) {
            editor = new ProjectEditor(mWorkspace, project);
            connect(editor, &ProjectEditor::projectEditorClosed, this, &ControlPanel::projectEditorClosed);
            connect(editor, &ProjectEditor::showControlPanelClicked, this, &ControlPanel::showControlPanel);
            mOpenProjectEditors.insert(project.getFilepath().toUnique().toStr(), editor);
            mWorkspace.setLastRecentlyUsedProject(project.getFilepath());
        }
        editor->showAllRequiredEditors();
        return editor;
    }
    catch (UserCanceled& e)
    {
        // do nothing
        return nullptr;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Could not open project"), e.getMsg());
        return nullptr;
    }
}

ProjectEditor* ControlPanel::openProject(const FilePath& filepath) noexcept
{
    try
    {
        ProjectEditor* editor = getOpenProject(filepath);
        if (!editor)
        {
            Project* project = new Project(filepath, false);
            editor = new ProjectEditor(mWorkspace, *project);
            connect(editor, &ProjectEditor::projectEditorClosed, this, &ControlPanel::projectEditorClosed);
            connect(editor, &ProjectEditor::showControlPanelClicked, this, &ControlPanel::showControlPanel);
            mOpenProjectEditors.insert(filepath.toUnique().toStr(), editor);
            mWorkspace.setLastRecentlyUsedProject(filepath);
        }
        editor->showAllRequiredEditors();
        return editor;
    }
    catch (UserCanceled& e)
    {
        // do nothing
        return nullptr;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Could not open project"), e.getMsg());
        return nullptr;
    }
}

bool ControlPanel::closeProject(ProjectEditor& editor, bool askForSave) noexcept
{
    Q_ASSERT(mOpenProjectEditors.contains(editor.getProject().getFilepath().toUnique().toStr()));
    return editor.closeAndDestroy(askForSave, this); // this will implicitly call the slot "projectEditorClosed()"!
}

bool ControlPanel::closeProject(const FilePath& filepath, bool askForSave) noexcept
{
    ProjectEditor* editor = getOpenProject(filepath);
    if (editor)
        return closeProject(*editor, askForSave);
    else
        return false;
}

bool ControlPanel::closeAllProjects(bool askForSave) noexcept
{
    bool success = true;
    foreach (ProjectEditor* editor, mOpenProjectEditors)
    {
        if (!closeProject(*editor, askForSave))
            success = false;
    }
    return success;
}

ProjectEditor* ControlPanel::getOpenProject(const FilePath& filepath) const noexcept
{
    if (mOpenProjectEditors.contains(filepath.toUnique().toStr()))
        return mOpenProjectEditors.value(filepath.toUnique().toStr());
    else
        return nullptr;
}

/*****************************************************************************************
 *  Library Management
 ****************************************************************************************/

void ControlPanel::openLibraryEditor(QSharedPointer<library::Library> lib) noexcept
{
    using library::editor::LibraryEditor;
    LibraryEditor* editor = mOpenLibraryEditors.value(lib.data());
    if (!editor) {
        try {
            editor = new LibraryEditor(mWorkspace, lib);
            connect(editor, &LibraryEditor::destroyed,
                    this, &ControlPanel::libraryEditorDestroyed);
            mOpenLibraryEditors.insert(lib.data(), editor);
        } catch (const Exception& e) {
            QMessageBox::critical(this, tr("Error"), e.getMsg());
        }
    }
    editor->show();
    editor->raise();
    editor->activateWindow();
}

void ControlPanel::libraryEditorDestroyed() noexcept
{
    using library::editor::LibraryEditor;
    // Note: Actually we should dynamic_cast the QObject* to LibraryEditor*, but as this
    // slot is called in the destructor of QObject (base class of LibraryEditor), the
    // dynamic_cast does no longer work at this point, so a static_cast is used instead ;)
    LibraryEditor* editor = static_cast<LibraryEditor*>(QObject::sender()); Q_ASSERT(editor);
    library::Library* library = mOpenLibraryEditors.key(editor); Q_ASSERT(library);
    mOpenLibraryEditors.remove(library);
}

bool ControlPanel::closeAllLibraryEditors(bool askForSave) noexcept
{
    using library::editor::LibraryEditor;
    bool success = true;
    foreach (LibraryEditor* editor, mOpenLibraryEditors) {
        if (editor->closeAndDestroy(askForSave)) {
            delete editor; // this calls the slot "libraryEditorDestroyed()"
        } else {
            success = false;
        }
    }
    return success;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void ControlPanel::projectEditorClosed() noexcept
{
    ProjectEditor* editor = dynamic_cast<ProjectEditor*>(QObject::sender());
    Q_ASSERT(editor); if (!editor) return;

    Project* project = &editor->getProject();
    mOpenProjectEditors.remove(project->getFilepath().toStr());
    delete project;
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void ControlPanel::on_actionAbout_triggered()
{
    QQuickView *view = new QQuickView(QUrl("qrc:/qml/dialogs/about.qml"));
    view->rootContext()->setContextProperty("view", view);
    view->rootContext()->setContextProperty("appVersion", qApp->getAppVersion().toPrettyStr(3));
    view->rootContext()->setContextProperty("gitVersion", qApp->getGitVersion());
    view->show();
}

void ControlPanel::on_actionNew_Project_triggered()
{
    newProject(mWorkspace.getProjectsPath());
}

void ControlPanel::on_actionOpen_Project_triggered()
{
    QSettings settings; // client settings
    QString lastOpenedFile = settings.value("controlpanel/last_open_project",
                             mWorkspace.getPath().toStr()).toString();

    FilePath filepath(QFileDialog::getOpenFileName(this, tr("Open Project"), lastOpenedFile,
                                    tr("LibrePCB project files (%1)").arg("*.lpp")));

    if (!filepath.isValid())
        return;

    settings.setValue("controlpanel/last_open_project", filepath.toNative());

    openProject(filepath);
}

void ControlPanel::on_actionOpen_Library_Manager_triggered()
{
    mLibraryManager->show();
    mLibraryManager->raise();
    mLibraryManager->activateWindow();
    mLibraryManager->updateRepositoryLibraryList();
}

void ControlPanel::on_actionClose_all_open_projects_triggered()
{
    closeAllProjects(true);
}

void ControlPanel::on_actionSwitch_Workspace_triggered()
{
    FilePath wsPath = Workspace::chooseWorkspacePath();
    if (!wsPath.isValid())
        return;

    Workspace::setMostRecentlyUsedWorkspacePath(wsPath);
    QMessageBox::information(this, tr("Workspace changed"),
        tr("The chosen workspace will be used after restarting the application."));
}

void ControlPanel::on_projectTreeView_clicked(const QModelIndex& index)
{
    FilePath fp(mWorkspace.getProjectTreeModel().filePath(index));
    if ((fp.getSuffix() == "lpp") || (fp.getFilename() == "README.md")) {
        showProjectReadmeInBrowser(fp.getParentDir());
    } else {
        showProjectReadmeInBrowser(fp);
    }
}

void ControlPanel::on_projectTreeView_doubleClicked(const QModelIndex& index)
{
    FilePath fp(mWorkspace.getProjectTreeModel().filePath(index));
    if (fp.isExistingDir()) {
        mUi->projectTreeView->setExpanded(index, !mUi->projectTreeView->isExpanded(index));
    } else if (fp.getSuffix() == "lpp") {
        openProject(fp);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fp.toStr()));
    }
}

void ControlPanel::on_projectTreeView_customContextMenuRequested(const QPoint& pos)
{
    // get clicked tree item filepath
    QModelIndex index = mUi->projectTreeView->indexAt(pos);
    FilePath fp = index.isValid()
                  ? FilePath(mWorkspace.getProjectTreeModel().filePath(index))
                  : mWorkspace.getProjectsPath();
    bool isProjectFile = Project::isProjectFile(fp);
    bool isProjectDir = Project::isProjectDirectory(fp);
    bool isInProjectDir = Project::isFilePathInsideProjectDirectory(fp);


    // build context menu with actions
    QMenu menu;
    enum Action {OpenProject, CloseProject, AddFavorite, RemoveFavorite, // on projects
                 NewProject, NewFolder,                                  // on folders
                 Open, Remove};                                          // on folders+files
    if (isProjectFile) {
        if (!getOpenProject(fp)) {
            menu.addAction(QIcon(":/img/actions/open.png"), tr("Open Project"))->setData(OpenProject);
            menu.setDefaultAction(menu.actions().last());
        } else {
            menu.addAction(QIcon(":/img/actions/close.png"), tr("Close Project"))->setData(CloseProject);
        }
        menu.addSeparator();
        if (mWorkspace.isFavoriteProject(fp)) {
            menu.addAction(QIcon(":/img/actions/bookmark.png"), tr("Remove from favorites"))->setData(RemoveFavorite);
        } else {
            menu.addAction(QIcon(":/img/actions/bookmark_gray.png"), tr("Add to favorites"))->setData(AddFavorite);
        }
    } else {
        menu.addAction(QIcon(":/img/actions/open.png"), tr("Open"))->setData(Open);
        if (fp.isExistingFile()) {menu.setDefaultAction(menu.actions().last());}
    }
    menu.addSeparator();
    if (fp.isExistingDir() && (!isProjectDir) && (!isInProjectDir)) {
        menu.addAction(QIcon(":/img/places/project_folder.png"), tr("New Project"))->setData(NewProject);
        menu.addAction(QIcon(":/img/actions/new_folder.png"), tr("New Folder"))->setData(NewFolder);
    }
    if (fp != mWorkspace.getProjectsPath()) {
        menu.addSeparator();
        menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove"))->setData(Remove);
    }

    // show context menu and execute the clicked action
    QAction* action = menu.exec(QCursor::pos());
    if (!action) return;
    switch (action->data().toInt()) {
        case OpenProject: openProject(fp); break;
        case CloseProject: closeProject(fp, true); break;
        case AddFavorite: mWorkspace.addFavoriteProject(fp); break;
        case RemoveFavorite: mWorkspace.removeFavoriteProject(fp); break;
        case NewProject: newProject(fp); break;
        case NewFolder: QDir(fp.toStr()).mkdir(QInputDialog::getText(this, tr("New Folder"), tr("Name:"))); break;
        case Open: QDesktopServices::openUrl(QUrl::fromLocalFile(fp.toStr())); break;
        case Remove: {
            QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Remove"),
                QString(tr("Are you really sure to remove following file or directory?\n\n"
                           "%1\n\nWarning: This cannot be undone!")).arg(fp.toNative()));
            if (btn == QMessageBox::Yes) {
                try {
                    if (fp.isExistingDir()) {
                        FileUtils::removeDirRecursively(fp);
                    } else {
                        FileUtils::removeFile(fp);
                    }
                } catch (const Exception& e) {
                    QMessageBox::critical(this, tr("Error"), e.getMsg());
                }
            }
            break;
        }
        default: qCritical() << "Unknown action triggered"; break;
    }
}

void ControlPanel::on_recentProjectsListView_entered(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    showProjectReadmeInBrowser(filepath.getParentDir());
}

void ControlPanel::on_favoriteProjectsListView_entered(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    showProjectReadmeInBrowser(filepath.getParentDir());
}

void ControlPanel::on_recentProjectsListView_clicked(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    openProject(filepath);
}

void ControlPanel::on_favoriteProjectsListView_clicked(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    openProject(filepath);
}

void ControlPanel::on_recentProjectsListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = mUi->recentProjectsListView->indexAt(pos);
    if (!index.isValid())
        return;

    bool isFavorite = mWorkspace.isFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));

    QMenu menu;
    QAction* action;
    if (isFavorite)
    {
        action = menu.addAction(QIcon(":/img/actions/bookmark.png"),
                                           tr("Remove from favorites"));
    }
    else
    {
        action = menu.addAction(QIcon(":/img/actions/bookmark_gray.png"),
                                           tr("Add to favorites"));
    }

    if (menu.exec(QCursor::pos()) == action)
    {
        if (isFavorite)
            mWorkspace.removeFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
        else
            mWorkspace.addFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
    }
}

void ControlPanel::on_favoriteProjectsListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = mUi->favoriteProjectsListView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu menu;
    QAction* removeAction = menu.addAction(QIcon(":/img/actions/cancel.png"),
                                           tr("Remove from favorites"));

    if (menu.exec(QCursor::pos()) == removeAction)
        mWorkspace.removeFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
}

void ControlPanel::on_actionRescanLibrary_triggered()
{
    mWorkspace.getLibraryDb().startLibraryRescan();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace application
} // namespace librepcb
