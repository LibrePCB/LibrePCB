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
#include "controlpanel.h"

#include "../../dialogs/directorylockhandlerdialog.h"
#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "../../library/libraryeditor.h"
#include "../../project/newprojectwizard/newprojectwizard.h"
#include "../../project/projecteditor.h"
#include "../../utils/menubuilder.h"
#include "../../utils/standardeditorcommandhandler.h"
#include "../../workspace/desktopservices.h"
#include "../../workspace/librarymanager/librarymanager.h"
#include "../initializeworkspacewizard/initializeworkspacewizard.h"
#include "../projectlibraryupdater/projectlibraryupdater.h"
#include "../workspacesettingsdialog.h"
#include "favoriteprojectsmodel.h"
#include "markdownconverter.h"
#include "projecttreemodel.h"
#include "recentprojectsmodel.h"
#include "ui_controlpanel.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ControlPanel::ControlPanel(Workspace& workspace, bool fileFormatIsOutdated)
  : QMainWindow(nullptr),
    mWorkspace(workspace),
    mUi(new Ui::ControlPanel),
    mStandardCommandHandler(
        new StandardEditorCommandHandler(mWorkspace.getSettings(), this)),
    mLibraryManager(new LibraryManager(mWorkspace, this)) {
  mUi->setupUi(this);
  setWindowTitle(
      tr("Control Panel - LibrePCB %1").arg(qApp->applicationVersion()));

  // show workspace path in status bar
  QString wsPath = mWorkspace.getPath().toNative();
  QLabel* statusBarLabel = new QLabel(tr("Workspace: %1").arg(wsPath));
  mUi->statusBar->addWidget(statusBarLabel, 1);

  // initialize status bar
  mUi->statusBar->setFields(StatusBar::ProgressBar);
  mUi->statusBar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanProgressUpdate,
          mUi->statusBar, &StatusBar::setProgressBarPercent,
          Qt::QueuedConnection);

  // Setup actions and menus.
  createActions();
  createMenus();

  // Show warning if the workspace has already been opened with a higher
  // file format version.
  mUi->lblWarnForNewerAppVersions->setVisible(fileFormatIsOutdated);

  // hide warning about missing libraries, but update visibility each time the
  // workspace library was scanned
  mUi->lblWarnForNoLibraries->setVisible(false);
  connect(mUi->lblWarnForNoLibraries, &QLabel::linkActivated, this,
          &ControlPanel::openLibraryManager);
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &ControlPanel::updateNoLibrariesWarningVisibility);

  // connect some actions which are created with the Qt Designer
  connect(mUi->openProjectButton, &QPushButton::clicked,
          mActionOpenProject.data(), &QAction::trigger);
  connect(mUi->newProjectButton, &QPushButton::clicked,
          mActionNewProject.data(), &QAction::trigger);
  connect(mUi->openLibraryManagerButton, &QPushButton::clicked,
          mActionLibraryManager.data(), &QAction::trigger);
  connect(mLibraryManager.data(), &LibraryManager::openLibraryEditorTriggered,
          this, &ControlPanel::openLibraryEditor);

  // load project models
  mRecentProjectsModel.reset(new RecentProjectsModel(mWorkspace));
  mFavoriteProjectsModel.reset(new FavoriteProjectsModel(mWorkspace));
  mProjectTreeModel.reset(new ProjectTreeModel(mWorkspace));

  // build projects file tree
  mUi->projectTreeView->setModel(mProjectTreeModel.data());
  mUi->projectTreeView->setRootIndex(
      mProjectTreeModel->index(mWorkspace.getProjectsPath().toStr()));
  for (int i = 1; i < mUi->projectTreeView->header()->count(); ++i) {
    mUi->projectTreeView->hideColumn(i);
  }

  // load recent and favorite project models
  mUi->recentProjectsListView->setModel(mRecentProjectsModel.data());
  mUi->favoriteProjectsListView->setModel(mFavoriteProjectsModel.data());

  loadSettings();

  // slightly delay opening projects to make sure the control panel window goes
  // to background (schematic editor should be the top most window)
  QTimer::singleShot(10, this, &ControlPanel::openProjectsPassedByCommandLine);

  // start scanning the workspace library (asynchronously)
  mWorkspace.getLibraryDb().startLibraryRescan();
}

ControlPanel::~ControlPanel() {
  mProjectLibraryUpdater.reset();
  closeAllProjects(false);
  closeAllLibraryEditors(false);
  mLibraryManager.reset();
  mUi.reset();
}

void ControlPanel::closeEvent(QCloseEvent* event) {
  // close all projects, unsaved projects will ask for saving
  if (!closeAllProjects(true)) {
    event->ignore();
    return;  // do NOT close the application, there are still open projects!
  }

  // close all library editors, unsaved libraries will ask for saving
  if (!closeAllLibraryEditors(true)) {
    event->ignore();
    return;  // do NOT close the application, there are still open library
             // editors!
  }

  saveSettings();

  QMainWindow::closeEvent(event);

  // if the control panel is closed, we will quit the whole application
  QApplication::quit();
}

void ControlPanel::showControlPanel() noexcept {
  show();
  raise();
  activateWindow();
}

void ControlPanel::openProjectLibraryUpdater(const FilePath& project) noexcept {
  mProjectLibraryUpdater.reset(
      new ProjectLibraryUpdater(mWorkspace, project, *this));
  mProjectLibraryUpdater->show();
}

/*******************************************************************************
 *  General private methods
 ******************************************************************************/

void ControlPanel::createActions() noexcept {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  mActionLibraryManager.reset(cmd.libraryManager.createAction(
      this, this, &ControlPanel::openLibraryManager,
      EditorCommand::ActionFlag::ApplicationShortcut));
  mActionWorkspaceSettings.reset(cmd.workspaceSettings.createAction(
      this, this,
      [this]() {
        WorkspaceSettingsDialog dialog(mWorkspace, this);
        dialog.exec();
      },
      EditorCommand::ActionFlag::ApplicationShortcut));
  mActionRescanLibraries.reset(cmd.workspaceLibrariesRescan.createAction(
      this, &mWorkspace.getLibraryDb(),
      &WorkspaceLibraryDb::startLibraryRescan));
  mActionSwitchWorkspace.reset(cmd.workspaceSwitch.createAction(
      this, this, &ControlPanel::switchWorkspace));
  mActionNewProject.reset(
      cmd.projectNew.createAction(this, this, [this]() { newProject(); }));
  mActionOpenProject.reset(
      cmd.projectOpen.createAction(this, this, [this]() { openProject(); }));
  mActionCloseAllProjects.reset(cmd.projectCloseAll.createAction(
      this, this, [this]() { closeAllProjects(true); },
      EditorCommand::ActionFlag::ApplicationShortcut));
  mActionAboutLibrePcb.reset(cmd.aboutLibrePcb.createAction(
      this, mStandardCommandHandler.data(),
      &StandardEditorCommandHandler::aboutLibrePcb));
  mActionAboutQt.reset(
      cmd.aboutQt.createAction(this, qApp, &QApplication::aboutQt));
  mActionOnlineDocumentation.reset(cmd.documentationOnline.createAction(
      this, mStandardCommandHandler.data(),
      &StandardEditorCommandHandler::onlineDocumentation));
  mActionWebsite.reset(
      cmd.website.createAction(this, mStandardCommandHandler.data(),
                               &StandardEditorCommandHandler::website));
  mActionQuit.reset(cmd.applicationQuit.createAction(
      this, qApp, &Application::quitTriggered));
}

void ControlPanel::createMenus() noexcept {
  MenuBuilder mb(mUi->menuBar);

  // File.
  mb.newMenu(&MenuBuilder::createFileMenu);
  mb.addAction(mActionNewProject);
  mb.addAction(mActionOpenProject);
  mb.addAction(mActionCloseAllProjects);
  mb.addSeparator();
  mb.addAction(mActionSwitchWorkspace);
  mb.addSeparator();
  mb.addAction(mActionQuit);

  // Extras.
  mb.newMenu(&MenuBuilder::createExtrasMenu);
  mb.addAction(mActionRescanLibraries);
  mb.addAction(mActionLibraryManager);
  mb.addSeparator();
  mb.addAction(mActionWorkspaceSettings);

  // Help.
  mb.newMenu(&MenuBuilder::createHelpMenu);
  mb.addAction(mActionOnlineDocumentation);
  mb.addAction(mActionWebsite);
  mb.addAction(mActionAboutLibrePcb);
  mb.addAction(mActionAboutQt);
}

void ControlPanel::saveSettings() {
  QSettings clientSettings;
  clientSettings.beginGroup("controlpanel");

  // main window
  clientSettings.setValue("window_geometry", saveGeometry());
  clientSettings.setValue("window_state", saveState());
  clientSettings.setValue("splitter_h_state", mUi->splitter_h->saveState());
  clientSettings.setValue("splitter_v_state", mUi->splitter_v->saveState());

  // projects treeview (expanded items)
  if (ProjectTreeModel* model =
          dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model())) {
    QStringList list;
    foreach (QModelIndex index, model->getPersistentIndexList()) {
      if (mUi->projectTreeView->isExpanded(index)) {
        list.append(
            FilePath(model->filePath(index)).toRelative(mWorkspace.getPath()));
      }
    }
    clientSettings.setValue("expanded_projecttreeview_items",
                            QVariant::fromValue(list));
  }

  clientSettings.endGroup();
}

void ControlPanel::loadSettings() {
  QSettings clientSettings;
  clientSettings.beginGroup("controlpanel");

  // main window
  restoreGeometry(clientSettings.value("window_geometry").toByteArray());
  restoreState(clientSettings.value("window_state").toByteArray());
  mUi->splitter_h->restoreState(
      clientSettings.value("splitter_h_state").toByteArray());
  mUi->splitter_v->restoreState(
      clientSettings.value("splitter_v_state").toByteArray());

  // projects treeview (expanded items)
  if (ProjectTreeModel* model =
          dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model())) {
    QStringList list =
        clientSettings.value("expanded_projecttreeview_items").toStringList();
    foreach (QString item, list) {
      FilePath filepath = FilePath::fromRelative(mWorkspace.getPath(), item);
      QModelIndex index = model->index(filepath.toStr());
      mUi->projectTreeView->setExpanded(index, true);
    }
  }

  clientSettings.endGroup();
}

void ControlPanel::updateNoLibrariesWarningVisibility() noexcept {
  bool showWarning = false;
  try {
    showWarning = mWorkspace.getLibraryDb().getAll<Library>().isEmpty();
  } catch (const Exception& e) {
    qCritical() << "Could not get library list:" << e.getMsg();
  }
  mUi->lblWarnForNoLibraries->setVisible(showWarning);
}

void ControlPanel::openLibraryManager() noexcept {
  mLibraryManager->show();
  mLibraryManager->raise();
  mLibraryManager->activateWindow();
  mLibraryManager->updateRepositoryLibraryList();
}

void ControlPanel::switchWorkspace() noexcept {
  InitializeWorkspaceWizard wizard(true, this);
  try {
    wizard.setWorkspacePath(mWorkspace.getPath());
  } catch (const Exception& e) {
    qWarning() << "ControlPanel: Failed to set current workspace path.";
  }
  if ((wizard.exec() == QDialog::Accepted) &&
      (wizard.getWorkspacePath().isValid())) {
    Workspace::setMostRecentlyUsedWorkspacePath(wizard.getWorkspacePath());
    QMessageBox::information(this, tr("Workspace changed"),
                             tr("The chosen workspace will be used after "
                                "restarting the application."));
  }
}

void ControlPanel::showProjectReadmeInBrowser(
    const FilePath& projectFilePath) noexcept {
  if (projectFilePath.isValid()) {
    FilePath readmeFilePath = projectFilePath.getPathTo("README.md");
    mUi->textBrowser->setSearchPaths(QStringList(projectFilePath.toStr()));
    mUi->textBrowser->setHtml(
        MarkdownConverter::convertMarkdownToHtml(readmeFilePath));
  } else {
    mUi->textBrowser->clear();
  }
}

/*******************************************************************************
 *  Project Management
 ******************************************************************************/

ProjectEditor* ControlPanel::newProject(FilePath parentDir) noexcept {
  if (!parentDir.isValid()) {
    parentDir = mWorkspace.getProjectsPath();
  }

  NewProjectWizard wizard(mWorkspace, this);
  wizard.setLocation(parentDir);
  if (wizard.exec() == QWizard::Accepted) {
    try {
      QScopedPointer<Project> project(wizard.createProject());  // can throw
      return openProject(*project.take());
    } catch (Exception& e) {
      QMessageBox::critical(this, tr("Could not create project"), e.getMsg());
    }
  }
  return nullptr;
}

ProjectEditor* ControlPanel::openProject(Project& project) noexcept {
  try {
    ProjectEditor* editor = getOpenProject(project.getFilepath());
    if (!editor) {
      editor = new ProjectEditor(mWorkspace, project);
      connect(editor, &ProjectEditor::projectEditorClosed, this,
              &ControlPanel::projectEditorClosed);
      connect(editor, &ProjectEditor::showControlPanelClicked, this,
              &ControlPanel::showControlPanel);
      connect(editor, &ProjectEditor::openProjectLibraryUpdaterClicked, this,
              &ControlPanel::openProjectLibraryUpdater);
      mOpenProjectEditors.insert(project.getFilepath().toUnique().toStr(),
                                 editor);
      mRecentProjectsModel->setLastRecentProject(project.getFilepath());
    }
    editor->showAllRequiredEditors();
    return editor;
  } catch (UserCanceled& e) {
    // do nothing
    return nullptr;
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Could not open project"), e.getMsg());
    return nullptr;
  }
}

ProjectEditor* ControlPanel::openProject(FilePath filepath) noexcept {
  if (!filepath.isValid()) {
    QSettings settings;  // client settings
    QString lastOpenedFile = settings
                                 .value("controlpanel/last_open_project",
                                        mWorkspace.getPath().toStr())
                                 .toString();

    filepath = FilePath(FileDialog::getOpenFileName(
        this, tr("Open Project"), lastOpenedFile,
        tr("LibrePCB project files (%1)").arg("*.lpp")));
    if (!filepath.isValid()) return nullptr;

    settings.setValue("controlpanel/last_open_project", filepath.toNative());
  }

  try {
    ProjectEditor* editor = getOpenProject(filepath);
    if (!editor) {
      // Opening the project can take some time, use wait cursor to provide
      // immediate UI feedback.
      setCursor(Qt::WaitCursor);
      auto cursorScopeGuard = scopeGuard([this]() { unsetCursor(); });

      std::shared_ptr<TransactionalFileSystem> fs =
          TransactionalFileSystem::openRW(
              filepath.getParentDir(), &askForRestoringBackup,
              DirectoryLockHandlerDialog::createDirectoryLockCallback());
      Project* project = new Project(std::unique_ptr<TransactionalDirectory>(
                                         new TransactionalDirectory(fs)),
                                     filepath.getFilename());
      editor = new ProjectEditor(mWorkspace, *project);
      connect(editor, &ProjectEditor::projectEditorClosed, this,
              &ControlPanel::projectEditorClosed);
      connect(editor, &ProjectEditor::showControlPanelClicked, this,
              &ControlPanel::showControlPanel);
      connect(editor, &ProjectEditor::openProjectLibraryUpdaterClicked, this,
              &ControlPanel::openProjectLibraryUpdater);
      mOpenProjectEditors.insert(filepath.toUnique().toStr(), editor);

      // Delay updating the last opened project to avoid an issue when
      // double-clicking: https://github.com/LibrePCB/LibrePCB/issues/293
      QTimer::singleShot(500, this, [this, filepath]() {
        mRecentProjectsModel->setLastRecentProject(filepath);
      });
    }
    editor->showAllRequiredEditors();
    return editor;
  } catch (UserCanceled& e) {
    // do nothing
    return nullptr;
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Could not open project"), e.getMsg());
    return nullptr;
  }
}

bool ControlPanel::closeProject(ProjectEditor& editor,
                                bool askForSave) noexcept {
  Q_ASSERT(mOpenProjectEditors.contains(
      editor.getProject().getFilepath().toUnique().toStr()));
  bool success = editor.closeAndDestroy(
      askForSave,
      this);  // this will implicitly call the slot "projectEditorClosed()"!
  if (success) {
    delete &editor;  // delete immediately to avoid locked projects when closing
                     // the app
  }
  return success;
}

bool ControlPanel::closeProject(const FilePath& filepath,
                                bool askForSave) noexcept {
  ProjectEditor* editor = getOpenProject(filepath);
  if (editor)
    return closeProject(*editor, askForSave);
  else
    return false;
}

bool ControlPanel::closeAllProjects(bool askForSave) noexcept {
  bool success = true;
  foreach (ProjectEditor* editor, mOpenProjectEditors) {
    if (!closeProject(*editor, askForSave)) success = false;
  }
  return success;
}

ProjectEditor* ControlPanel::getOpenProject(const FilePath& filepath) const
    noexcept {
  if (mOpenProjectEditors.contains(filepath.toUnique().toStr()))
    return mOpenProjectEditors.value(filepath.toUnique().toStr());
  else
    return nullptr;
}

bool ControlPanel::askForRestoringBackup(const FilePath& dir) {
  Q_UNUSED(dir);
  QMessageBox::StandardButton btn = QMessageBox::question(
      0, tr("Restore autosave backup?"),
      tr("It seems that the application crashed the last time you opened this "
         "project. Do you want to restore the last autosave backup?"),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Cancel);
  switch (btn) {
    case QMessageBox::Yes:
      return true;
    case QMessageBox::No:
      return false;
    default:
      throw UserCanceled(__FILE__, __LINE__);
  }
}

/*******************************************************************************
 *  Library Management
 ******************************************************************************/

void ControlPanel::openLibraryEditor(const FilePath& libDir) noexcept {
  LibraryEditor* editor = mOpenLibraryEditors.value(libDir);
  if (!editor) {
    try {
      bool remote = libDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());
      editor = new LibraryEditor(mWorkspace, libDir, remote);
      connect(editor, &LibraryEditor::destroyed, this,
              &ControlPanel::libraryEditorDestroyed);
      mOpenLibraryEditors.insert(libDir, editor);
    } catch (const UserCanceled& e) {
      // User requested to abort -> do nothing.
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
  }
  if (editor) {
    editor->show();
    editor->raise();
    editor->activateWindow();
  }
}

void ControlPanel::libraryEditorDestroyed() noexcept {
  // Note: Actually we should dynamic_cast the QObject* to LibraryEditor*, but
  // as this slot is called in the destructor of QObject (base class of
  // LibraryEditor), the dynamic_cast does no longer work at this point, so a
  // static_cast is used instead ;)
  LibraryEditor* editor = static_cast<LibraryEditor*>(QObject::sender());
  Q_ASSERT(editor);
  FilePath library = mOpenLibraryEditors.key(editor);
  Q_ASSERT(library.isValid());
  mOpenLibraryEditors.remove(library);
}

bool ControlPanel::closeAllLibraryEditors(bool askForSave) noexcept {
  bool success = true;
  foreach (LibraryEditor* editor, mOpenLibraryEditors) {
    if (editor->closeAndDestroy(askForSave)) {
      delete editor;  // this calls the slot "libraryEditorDestroyed()"
    } else {
      success = false;
    }
  }
  return success;
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void ControlPanel::openProjectsPassedByCommandLine() noexcept {
  // parse command line arguments and open all project files
  foreach (const QString& arg, qApp->arguments()) {
    FilePath filepath(arg);
    if ((filepath.isExistingFile()) && (filepath.getSuffix() == "lpp")) {
      openProject(filepath);
    }
  }
}

void ControlPanel::projectEditorClosed() noexcept {
  ProjectEditor* editor = dynamic_cast<ProjectEditor*>(QObject::sender());
  Q_ASSERT(editor);
  if (!editor) return;

  Project* project = &editor->getProject();
  Q_ASSERT(
      mOpenProjectEditors.contains(project->getFilepath().toUnique().toStr()));
  mOpenProjectEditors.remove(project->getFilepath().toUnique().toStr());
  delete project;
}

/*******************************************************************************
 *  Actions
 ******************************************************************************/

void ControlPanel::on_projectTreeView_clicked(const QModelIndex& index) {
  FilePath fp(mProjectTreeModel->filePath(index));
  if ((fp.getSuffix() == "lpp") || (fp.getFilename() == "README.md")) {
    showProjectReadmeInBrowser(fp.getParentDir());
  } else {
    showProjectReadmeInBrowser(fp);
  }
}

void ControlPanel::on_projectTreeView_doubleClicked(const QModelIndex& index) {
  FilePath fp(mProjectTreeModel->filePath(index));
  if (fp.isExistingDir()) {
    mUi->projectTreeView->setExpanded(index,
                                      !mUi->projectTreeView->isExpanded(index));
  } else if (fp.getSuffix() == "lpp") {
    openProject(fp);
  } else {
    QDesktopServices::openUrl(QUrl::fromLocalFile(fp.toStr()));
  }
}

void ControlPanel::on_projectTreeView_customContextMenuRequested(
    const QPoint& pos) {
  // get clicked tree item filepath
  QModelIndex index = mUi->projectTreeView->indexAt(pos);
  FilePath fp = index.isValid() ? FilePath(mProjectTreeModel->filePath(index))
                                : mWorkspace.getProjectsPath();
  bool isProjectFile = Project::isProjectFile(fp);
  bool isProjectDir = Project::isProjectDirectory(fp);
  bool isInProjectDir = Project::isFilePathInsideProjectDirectory(fp);

  // build context menu with actions
  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  if (isProjectFile) {
    if (!getOpenProject(fp)) {
      mb.addAction(cmd.itemOpen.createAction(
                       &menu, this, [this, fp]() { openProject(fp); },
                       EditorCommand::ActionFlag::NoShortcuts),
                   MenuBuilder::Flag::DefaultAction);
    } else {
      mb.addAction(cmd.projectClose.createAction(
          &menu, this, [this, fp]() { closeProject(fp, true); },
          EditorCommand::ActionFlag::NoShortcuts));
    }
    mb.addSeparator();
    if (mFavoriteProjectsModel->isFavoriteProject(fp)) {
      mb.addAction(cmd.favoriteRemove.createAction(
          &menu, this,
          [this, fp]() { mFavoriteProjectsModel->removeFavoriteProject(fp); },
          EditorCommand::ActionFlag::NoShortcuts));
    } else {
      mb.addAction(cmd.favoriteAdd.createAction(
          &menu, this,
          [this, fp]() { mFavoriteProjectsModel->addFavoriteProject(fp); },
          EditorCommand::ActionFlag::NoShortcuts));
    }
    mb.addAction(cmd.projectLibraryUpdate.createAction(
        &menu, this, [this, fp]() { openProjectLibraryUpdater(fp); },
        EditorCommand::ActionFlag::NoShortcuts));
  } else {
    mb.addAction(
        cmd.itemOpen.createAction(&menu, this,
                                  [fp]() {
                                    QDesktopServices::openUrl(
                                        QUrl::fromLocalFile(fp.toStr()));
                                  },
                                  EditorCommand::ActionFlag::NoShortcuts),
        MenuBuilder::Flag::DefaultAction);
  }
  mb.addSeparator();
  if (fp.isExistingDir() && (!isProjectDir) && (!isInProjectDir)) {
    mb.addAction(cmd.projectNew.createAction(
        &menu, this, [this, fp]() { newProject(fp); },
        EditorCommand::ActionFlag::NoShortcuts));
    mb.addAction(cmd.folderNew.createAction(
        &menu, this,
        [this, fp]() {
          QDir(fp.toStr())
              .mkdir(
                  QInputDialog::getText(this, tr("New Folder"), tr("Name:")));
        },
        EditorCommand::ActionFlag::NoShortcuts));
  }
  if (fp != mWorkspace.getProjectsPath()) {
    mb.addSeparator();
    mb.addAction(cmd.remove.createAction(
        &menu, this,
        [this, fp]() {
          QMessageBox::StandardButton btn = QMessageBox::question(
              this, tr("Remove"),
              tr("Are you really sure to remove following file or "
                 "directory?\n\n"
                 "%1\n\nWarning: This cannot be undone!")
                  .arg(fp.toNative()));
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
            // something was removed -> update lists of recent and favorite
            // projects
            mRecentProjectsModel->updateVisibleProjects();
            mFavoriteProjectsModel->updateVisibleProjects();
          }
        },
        EditorCommand::ActionFlag::NoShortcuts));
  }

  // show context menu and execute the clicked action
  menu.exec(QCursor::pos());
}

void ControlPanel::on_recentProjectsListView_entered(const QModelIndex& index) {
  FilePath filepath(index.data(Qt::UserRole).toString());
  showProjectReadmeInBrowser(filepath.getParentDir());
}

void ControlPanel::on_favoriteProjectsListView_entered(
    const QModelIndex& index) {
  FilePath filepath(index.data(Qt::UserRole).toString());
  showProjectReadmeInBrowser(filepath.getParentDir());
}

void ControlPanel::on_recentProjectsListView_clicked(const QModelIndex& index) {
  FilePath filepath(index.data(Qt::UserRole).toString());
  openProject(filepath);
}

void ControlPanel::on_favoriteProjectsListView_clicked(
    const QModelIndex& index) {
  FilePath filepath(index.data(Qt::UserRole).toString());
  openProject(filepath);
}

void ControlPanel::on_recentProjectsListView_customContextMenuRequested(
    const QPoint& pos) {
  QModelIndex index = mUi->recentProjectsListView->indexAt(pos);
  if (!index.isValid()) return;

  bool isFavorite = mFavoriteProjectsModel->isFavoriteProject(
      FilePath(index.data(Qt::UserRole).toString()));

  FilePath fp = FilePath(index.data(Qt::UserRole).toString());
  if (!fp.isValid()) return;

  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mb.addAction(
      cmd.itemOpen.createAction(&menu, this, [this, fp]() { openProject(fp); },
                                EditorCommand::ActionFlag::NoShortcuts),
      MenuBuilder::Flag::DefaultAction);
  mb.addSeparator();
  if (isFavorite) {
    mb.addAction(cmd.favoriteRemove.createAction(
        &menu, this,
        [this, fp]() { mFavoriteProjectsModel->removeFavoriteProject(fp); },
        EditorCommand::ActionFlag::NoShortcuts));
  } else {
    mb.addAction(cmd.favoriteAdd.createAction(
        &menu, this,
        [this, fp]() { mFavoriteProjectsModel->addFavoriteProject(fp); },
        EditorCommand::ActionFlag::NoShortcuts));
  }
  mb.addAction(cmd.projectLibraryUpdate.createAction(
      &menu, this, [this, fp]() { openProjectLibraryUpdater(fp); }));
  menu.exec(QCursor::pos());
}

void ControlPanel::on_favoriteProjectsListView_customContextMenuRequested(
    const QPoint& pos) {
  QModelIndex index = mUi->favoriteProjectsListView->indexAt(pos);
  if (!index.isValid()) return;

  FilePath fp = FilePath(index.data(Qt::UserRole).toString());
  if (!fp.isValid()) return;

  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mb.addAction(
      cmd.itemOpen.createAction(&menu, this, [this, fp]() { openProject(fp); },
                                EditorCommand::ActionFlag::NoShortcuts),
      MenuBuilder::Flag::DefaultAction);
  mb.addSeparator();
  mb.addAction(cmd.favoriteRemove.createAction(
      &menu, this,
      [this, fp]() { mFavoriteProjectsModel->removeFavoriteProject(fp); },
      EditorCommand::ActionFlag::NoShortcuts));
  mb.addAction(cmd.projectLibraryUpdate.createAction(
      &menu, this, [this, fp]() { openProjectLibraryUpdater(fp); }));
  menu.exec(QCursor::pos());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
