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
#include "guiapplication.h"

#include "dialogs/directorylockhandlerdialog.h"
#include "dialogs/filedialog.h"
#include "library/libraryeditor.h"
#include "mainwindow.h"
#include "notification.h"
#include "notificationsmodel.h"
#include "project/newprojectwizard/newprojectwizard.h"
#include "project/projecteditor.h"
#include "utils/slinthelpers.h"
#include "workspace/desktopintegration.h"
#include "workspace/desktopservices.h"
#include "workspace/initializeworkspacewizard/initializeworkspacewizard.h"
#include "workspace/librarymanager/librarymanager.h"
#include "workspace/projectlibraryupdater/projectlibraryupdater.h"
#include "workspace/quickaccessmodel.h"
#include "workspace/workspacesettingsdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GuiApplication::GuiApplication(Workspace& ws, bool fileFormatIsOutdated,
                               QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mNotifications(new NotificationsModel(ws)),
    mQuickAccessModel(new QuickAccessModel(ws)),
    mLibraryManager(new LibraryManager(ws)) {
  // Open windows.
  QSettings cs;
  for (const QString& idStr : cs.value("global/windows").toStringList()) {
    if (int id = idStr.toInt()) {
      createNewWindow(id);
    }
  }
  if (mWindows.isEmpty()) {
    createNewWindow();
  }

  // Setup quick access.
  connect(mQuickAccessModel.get(), &QuickAccessModel::openFileTriggered, this,
          [this](const FilePath& fp) { openFile(fp, qApp->activeWindow()); });

  // Setup library manager.
  connect(mLibraryManager.data(), &LibraryManager::openLibraryEditorTriggered,
          this, &GuiApplication::openLibraryEditor);

  // Connect notification signals.
  const qint64 startupTime = QDateTime::currentMSecsSinceEpoch();
  connect(
      mNotifications.get(), &NotificationsModel::autoPopUpRequested, this,
      [this, startupTime]() {
        if (auto w = getCurrentWindow()) {
          // It looks ugly if the notifications pop up immediately when the
          // whole window is just opened, so we delay it a bit. Not the best
          // implementation, probably this could be improved somehow...
          const qint64 delay =
              std::max(startupTime + 500 - QDateTime::currentMSecsSinceEpoch(),
                       qint64(0));
          QTimer::singleShot(delay, w.get(), &MainWindow::popUpNotifications);
        }
      });

  // Show warning if the workspace has already been opened with a higher
  // file format version.
  if (fileFormatIsOutdated) {
    mNotifications->push(std::make_shared<Notification>(
        ui::NotificationType::Warning, tr("Older Application Version Used"),
        tr("This workspace was already used with a newer version of LibrePCB. "
           "This is fine, just note that any changes in libraries and "
           "workspace "
           "settings won't be available in newer versions of LibrePCB."),
        QString(),
        QString("WORKSPACE_V%1_OPENED_WITH_NEWER_VERSION")
            .arg(Application::getFileFormatVersion().toStr()),
        true));
  }

  // Setup warning about missing libraries, and update visibility each time the
  // workspace library was scanned.
  mNotificationNoLibrariesInstalled.reset(new Notification(
      ui::NotificationType::Tip, tr("No Libraries Installed"),
      tr("This workspace does not contain any libraries, which are essential "
         "to create and modify projects. You should open the libraries panel "
         "to add some libraries."),
      tr("Open Library Manager"),
      QString("WORKSPACE_V%1_HAS_NO_LIBRARIES")
          .arg(Application::getFileFormatVersion().toStr()),
      true));
  connect(mNotificationNoLibrariesInstalled.get(), &Notification::buttonClicked,
          this, &GuiApplication::openLibraryManager);
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &GuiApplication::updateNoLibrariesInstalledNotification);
  updateNoLibrariesInstalledNotification();

  // Suggest to install the desktop integration, if available.
  mNotificationDesktopIntegration.reset(new Notification(
      ui::NotificationType::Tip, tr("Application is Not Installed"),
      tr("This application executable does not seem to be integrated into your "
         "desktop environment. If desired, install it now to allow opening "
         "LibrePCB projects through the file manager. Click the button for "
         "details, or do it from the preferences dialog at any time."),
      tr("Install Desktop Integration") % "...",
      "DESKTOP_INTEGRATION_NOT_INSTALLED", true));
  connect(mNotificationDesktopIntegration.get(), &Notification::buttonClicked,
          this, [this]() {
            DesktopIntegration::execDialog(DesktopIntegration::Mode::Install,
                                           qApp->activeWindow());
            updateDesktopIntegrationNotification();
          });
  updateDesktopIntegrationNotification();

  // Show a notification during workspace libraries rescan.
  connect(
      &mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanStarted, this,
      [this]() {
        std::shared_ptr<Notification> n = std::make_shared<Notification>(
            ui::NotificationType::Progress, tr("Scanning Libraries") % "...",
            tr("The internal libraries database is beeing updated. This may "
               "take a few minutes and in the mean time you might see outdated "
               "information about libraries."),
            QString(), QString(), false);
        connect(&mWorkspace.getLibraryDb(),
                &WorkspaceLibraryDb::scanProgressUpdate, n.get(),
                &Notification::setProgress);
        connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanFinished,
                n.get(), &Notification::dismiss);
        mNotifications->push(n);
      });

  // If the library rescan failed, show a notification error.
  connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanFailed, this,
          [this](const QString& err) {
            std::shared_ptr<Notification> n = std::make_shared<Notification>(
                ui::NotificationType::Critical, tr("Scanning Libraries Failed"),
                err, QString(), QString(), true);
            connect(&mWorkspace.getLibraryDb(),
                    &WorkspaceLibraryDb::scanStarted, n.get(),
                    &Notification::dismiss);
            mNotifications->push(n);
          });

  // Configure window saving countdown timer.
  mSaveOpenedWindowsCountdown.setSingleShot(true);
  connect(&mSaveOpenedWindowsCountdown, &QTimer::timeout, this, [this]() {
    QStringList ids;
    for (const auto& win : mWindows) {
      ids.append(QString::number(win->getId()));
    }
    std::sort(ids.begin(), ids.end());

    QSettings cs;
    cs.setValue("global/windows", ids);
    qDebug().noquote() << "Saved opened window IDs:" << ids.join(", ");
  });

  // slightly delay opening projects to make sure the control panel window goes
  // to background (schematic editor should be the top most window)
  QTimer::singleShot(10, this,
                     &GuiApplication::openProjectsPassedByCommandLine);

  // To allow opening files by the MacOS Finder, install event filter.
  qApp->installEventFilter(this);

  // Start library rescan.
  mWorkspace.getLibraryDb().startLibraryRescan();
}

GuiApplication::~GuiApplication() noexcept {
  mProjectLibraryUpdater.reset();
  closeAllLibraryEditors(false);
  closeAllProjects(false, nullptr);
  mLibraryManager.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GuiApplication::openFile(const FilePath& fp, QWidget* parent) noexcept {
  if ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz")) {
    openProject(fp, parent);
  } else if (fp.isValid()) {
    DesktopServices ds(mWorkspace.getSettings());
    ds.openLocalPath(fp);
  }
}

void GuiApplication::switchWorkspace(QWidget* parent) noexcept {
  InitializeWorkspaceWizard wizard(true, parent);
  wizard.setWindowModality(Qt::WindowModal);
  try {
    wizard.setWorkspacePath(mWorkspace.getPath());
  } catch (const Exception& e) {
    qWarning() << "Failed to prepare workspace switching:" << e.getMsg();
  }
  if ((wizard.exec() == QDialog::Accepted) &&
      (wizard.getWorkspacePath().isValid())) {
    Workspace::setMostRecentlyUsedWorkspacePath(wizard.getWorkspacePath());
    QMessageBox::information(parent, tr("Workspace changed"),
                             tr("The chosen workspace will be used after "
                                "restarting the application."));
  }
}

void GuiApplication::execWorkspaceSettingsDialog(QWidget* parent) noexcept {
  WorkspaceSettingsDialog dlg(mWorkspace, parent);
  connect(&dlg, &WorkspaceSettingsDialog::desktopIntegrationStatusChanged, this,
          &GuiApplication::updateDesktopIntegrationNotification);
  dlg.exec();
}

void GuiApplication::openLibraryManager() noexcept {
  mLibraryManager->show();
  mLibraryManager->raise();
  mLibraryManager->activateWindow();
  mLibraryManager->updateOnlineLibraryList();
}

void GuiApplication::addExampleProjects(QWidget* parent) noexcept {
  const QString msg =
      tr("This downloads some example projects from the internet and copies "
         "them into the workspace to help you evaluating LibrePCB with real "
         "projects.") %
      "\n\n" %
      tr("Once you don't need them anymore, just delete the examples "
         "directory to get rid of them.");
  const int ret =
      QMessageBox::information(parent, tr("Add Example Projects"), msg,
                               QMessageBox::Ok | QMessageBox::Cancel);
  if (ret == QMessageBox::Ok) {
    InitializeWorkspaceWizardContext ctx(parent);
    ctx.setWorkspacePath(mWorkspace.getPath());
    ctx.installExampleProjects();
  }
}

void GuiApplication::createProject(const FilePath& parentDir, bool eagleImport,
                                   QWidget* parent) noexcept {
  const NewProjectWizard::Mode mode = eagleImport
      ? NewProjectWizard::Mode::EagleImport
      : NewProjectWizard::Mode::NewProject;
  NewProjectWizard wizard(mWorkspace, mode, parent);
  wizard.setWindowModality(Qt::WindowModal);
  if (parentDir.isValid()) {
    wizard.setLocationOverride(parentDir);
  }
  if (wizard.exec() == QWizard::Accepted) {
    try {
      std::unique_ptr<Project> project = wizard.createProject();  // can throw
      const FilePath fp = project->getFilepath();
      project.reset();  // Release lock.
      openProject(fp, parent);
    } catch (const Exception& e) {
      QMessageBox::critical(parent, tr("Could not create project"), e.getMsg());
    }
  }
}

ProjectEditor* GuiApplication::openProject(FilePath filepath,
                                           QWidget* parent) noexcept {
  if (!filepath.isValid()) {
    QSettings settings;  // client settings
    QString lastOpenedFile = settings
                                 .value("controlpanel/last_open_project",
                                        mWorkspace.getPath().toStr())
                                 .toString();

    filepath = FilePath(FileDialog::getOpenFileName(
        parent, tr("Open Project"), lastOpenedFile,
        tr("LibrePCB project files (%1)").arg("*.lpp *.lppz")));
    if (!filepath.isValid()) return nullptr;

    settings.setValue("controlpanel/last_open_project", filepath.toNative());
  }

  try {
    ProjectEditor* editor = getOpenProject(filepath);
    if (!editor) {
      // Opening the project can take some time, use wait cursor to provide
      // immediate UI feedback.
      QGuiApplication::setOverrideCursor(Qt::WaitCursor);
      auto tmpCursor = scopeGuard(&QGuiApplication::restoreOverrideCursor);

      std::shared_ptr<TransactionalFileSystem> fs;
      QString projectFileName = filepath.getFilename();
      if (filepath.getSuffix() == "lppz") {
        fs = TransactionalFileSystem::openRO(
            FilePath::getRandomTempPath(),
            &TransactionalFileSystem::RestoreMode::no);
        fs->removeDirRecursively();  // 1) Get a clean initial state.
        fs->loadFromZip(filepath);  // 2) Load files from ZIP.
        foreach (const QString& fn, fs->getFiles()) {
          if (fn.endsWith(".lpp")) {
            projectFileName = fn;
          }
        }
      } else {
        fs = TransactionalFileSystem::openRW(
            filepath.getParentDir(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
      }
      ProjectLoader loader;
      std::unique_ptr<Project> project =
          loader.open(std::unique_ptr<TransactionalDirectory>(
                          new TransactionalDirectory(fs)),
                      projectFileName);  // can throw
      editor = new ProjectEditor(mWorkspace, *project.release(),
                                 loader.getUpgradeMessages());
      connect(editor, &ProjectEditor::projectEditorClosed, this,
              &GuiApplication::projectEditorClosed);
      connect(editor, &ProjectEditor::showControlPanelClicked, this, [this]() {
        if (auto win = mWindows.value(0)) {
          win->makeCurrentWindow();
        }
      });
      connect(editor, &ProjectEditor::openProjectLibraryUpdaterClicked, this,
              &GuiApplication::openProjectLibraryUpdater);
      mOpenProjectEditors.insert(filepath.toUnique().toStr(), editor);

      // Delay updating the last opened project to avoid an issue when
      // double-clicking: https://github.com/LibrePCB/LibrePCB/issues/293
      QTimer::singleShot(500, this, [this, filepath]() {
        mQuickAccessModel->pushRecentProject(filepath);
      });
    }
    editor->showAllRequiredEditors();
    return editor;
  } catch (const UserCanceled& e) {
    // do nothing
    return nullptr;
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Could not open project"), e.getMsg());
    return nullptr;
  }
}

void GuiApplication::createNewWindow(int id) noexcept {
  // Create Slint window.
  auto win = ui::AppWindow::create();

  // Set global data.
  const ui::Data& d = win->global<ui::Data>();
  d.set_preview_mode(false);
  d.set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  d.set_workspace_path(mWorkspace.getPath().toNative().toUtf8().data());
  d.set_notifications(mNotifications);
  d.set_quick_access_items(mQuickAccessModel);

  // Register global callbacks.
  const ui::Backend& b = win->global<ui::Backend>();
  b.on_open_url([this](const slint::SharedString& url) {
    DesktopServices ds(mWorkspace.getSettings());
    return ds.openUrl(QUrl(s2q(url)));
  });

  // Reuse next free window ID.
  if (id < 1) {
    id = 1;
    while (std::any_of(mWindows.begin(), mWindows.end(),
                       [id](const std::shared_ptr<MainWindow>& w) {
                         return w->getId() == id;
                       })) {
      ++id;
    }
  }

  // Build wrapper.
  auto mw = std::make_shared<MainWindow>(*this, win, id);
  connect(
      mw.get(), &MainWindow::aboutToClose, this,
      [this]() {
        MainWindow* mw = static_cast<MainWindow*>(sender());
        qDebug().nospace() << "Closed window with ID " << mw->getId() << ".";
        mWindows.removeIf(
            [mw](std::shared_ptr<MainWindow> p) { return p.get() == mw; });
        mw = nullptr;  // Not valid anymore!

        // Schedule saving number of opened windows.
        mSaveOpenedWindowsCountdown.start(10000);
      },
      Qt::QueuedConnection);
  mWindows.append(mw);
  qDebug().nospace() << "Opened new window with ID " << id << ".";

  // Schedule saving number of opened windows.
  mSaveOpenedWindowsCountdown.start(10000);
}

bool GuiApplication::requestClosingWindow(QWidget* parent) noexcept {
  if (mWindows.count() >= 2) {
    return true;
  }

  // Stop possibly scheduled window state saving as the user seems to be
  // closing the whole application, so we don't want to save this state.
  mSaveOpenedWindowsCountdown.stop();

  return closeAllLibraryEditors(true) && closeAllProjects(true, parent);
}

void GuiApplication::exec() {
  slint::run_event_loop();
}

void GuiApplication::quit(QPointer<QWidget> parent) noexcept {
  // Need to be delayed since this call is made from the object to be
  // deleted.
  QMetaObject::invokeMethod(
      this,
      [this, parent]() {
        if (closeAllLibraryEditors(true) && closeAllProjects(true, parent)) {
          mWindows.clear();
          slint::quit_event_loop();
        }
      },
      Qt::QueuedConnection);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

bool GuiApplication::eventFilter(QObject* watched, QEvent* event) noexcept {
  if (event->type() == QEvent::FileOpen) {
    QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
    qDebug() << "Received request to open file:" << openEvent->file();
    openProjectPassedByOs(openEvent->file());
    return true;
  }
  return QObject::eventFilter(watched, event);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GuiApplication::openProjectsPassedByCommandLine() noexcept {
  // Parse command line arguments and open all project files.
  // Note: Do not print a warning if the first argument is not a valid project,
  // since it might or might not be the application file path.
  const QStringList args = qApp->arguments();
  for (int i = 0; i < args.count(); ++i) {
    openProjectPassedByOs(args.at(i), i == 0);  // Silent on first item.
  }
}

void GuiApplication::openProjectPassedByOs(const QString& file,
                                           bool silent) noexcept {
  FilePath filepath(file);
  if ((filepath.isExistingFile()) &&
      ((filepath.getSuffix() == "lpp") || (filepath.getSuffix() == "lppz"))) {
    openProject(filepath, nullptr);
  } else if (!silent) {
    qWarning() << "Ignore invalid request to open project:" << file;
  }
}

ProjectEditor* GuiApplication::getOpenProject(
    const FilePath& filepath) const noexcept {
  if (mOpenProjectEditors.contains(filepath.toUnique().toStr()))
    return mOpenProjectEditors.value(filepath.toUnique().toStr());
  else
    return nullptr;
}

bool GuiApplication::askForRestoringBackup(const FilePath& dir) {
  Q_UNUSED(dir);
  QMessageBox::StandardButton btn = QMessageBox::question(
      qApp->activeWindow(), tr("Restore autosave backup?"),
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

bool GuiApplication::closeAllProjects(bool askForSave,
                                      QWidget* parent) noexcept {
  bool success = true;
  foreach (ProjectEditor* editor, mOpenProjectEditors) {
    if (editor->closeAndDestroy(askForSave, parent)) {
      delete editor;
    } else {
      success = false;
    }
  }
  return success;
}

void GuiApplication::projectEditorClosed() noexcept {
  ProjectEditor* editor = dynamic_cast<ProjectEditor*>(QObject::sender());
  Q_ASSERT(editor);
  if (!editor) return;

  const QString fp = mOpenProjectEditors.key(editor);
  Q_ASSERT(mOpenProjectEditors.contains(fp));
  mOpenProjectEditors.remove(fp);

  Project* project = &editor->getProject();
  delete project;
}

void GuiApplication::openProjectLibraryUpdater(
    const FilePath& project) noexcept {
  QPointer<ProjectEditor> editor = getOpenProject(project);
  const bool wasOpen = !editor.isNull();
  mProjectLibraryUpdater.reset(
      new ProjectLibraryUpdater(mWorkspace, project, [editor](const FilePath&) {
        if (editor) {
          return editor->closeAndDestroy(true);
        } else {
          return true;
        }
      }));
  if (wasOpen) {
    connect(mProjectLibraryUpdater.get(), &ProjectLibraryUpdater::finished,
            this, [this](const FilePath& fp) { openProject(fp, nullptr); });
  }
  mProjectLibraryUpdater->show();
}

void GuiApplication::openLibraryEditor(const FilePath& libDir) noexcept {
  LibraryEditor* editor = mOpenLibraryEditors.value(libDir);
  if (!editor) {
    try {
      bool remote = libDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());
      editor = new LibraryEditor(mWorkspace, libDir, remote);
      connect(editor, &LibraryEditor::destroyed, this,
              &GuiApplication::libraryEditorDestroyed);
      mOpenLibraryEditors.insert(libDir, editor);
    } catch (const UserCanceled& e) {
      // User requested to abort -> do nothing.
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }
  if (editor) {
    editor->show();
    editor->raise();
    editor->activateWindow();
  }
}

void GuiApplication::libraryEditorDestroyed() noexcept {
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

bool GuiApplication::closeAllLibraryEditors(bool askForSave) noexcept {
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

std::shared_ptr<MainWindow> GuiApplication::getCurrentWindow() noexcept {
  for (auto& win : mWindows) {
    if (win->isCurrentWindow()) {
      return win;
    }
  }
  // TODO: This does not work in every case yet, so we implement some fallback
  // as a workaround.
  return mWindows.value(mWindows.count() - 1);
}

void GuiApplication::updateNoLibrariesInstalledNotification() noexcept {
  if (!mNotificationNoLibrariesInstalled) return;

  bool showWarning = false;
  try {
    showWarning = mWorkspace.getLibraryDb().getAll<Library>().isEmpty();
  } catch (const Exception& e) {
    qCritical() << "Failed to get workspace library list:" << e.getMsg();
  }
  if (showWarning) {
    mNotifications->push(mNotificationNoLibrariesInstalled);
  } else {
    mNotificationNoLibrariesInstalled->dismiss();
  }
}

void GuiApplication::updateDesktopIntegrationNotification() noexcept {
  if (!mNotificationDesktopIntegration) return;

  if (DesktopIntegration::isSupported() &&
      (DesktopIntegration::getStatus() !=
       DesktopIntegration::Status::InstalledThis)) {
    mNotifications->push(mNotificationDesktopIntegration);
  } else {
    mNotificationDesktopIntegration->dismiss();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
