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
#include "mainwindow.h"

#include "dialogs/aboutdialog.h"
#include "editorcommandset.h"
#include "editorcommandsetupdater.h"
#include "guiapplication.h"
#include "mainwindowtestadapter.h"
#include "notificationsmodel.h"
#include "project/projectreadmerenderer.h"
#include "utils/editortoolbox.h"
#include "utils/standardeditorcommandhandler.h"
#include "workspace/filesystemmodel.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int id,
                       QObject* parent) noexcept
  : QObject(parent),
    mId(id),
    mSettingsPrefix(QString("window_%1").arg(mId)),
    mApp(app),
    mWindow(win),
    mWidget(static_cast<QWidget*>(slint::cbindgen_private::slint_qt_get_widget(
        &mWindow->window().window_handle()))),
    mProjectPreviewRenderer(new ProjectReadmeRenderer(this)),
    mTestAdapter(new MainWindowTestAdapter(app, mWidget)) {
  Q_ASSERT(mWidget);
  mWidget->setObjectName("mainWindow");

  // Register Slint callbacks.
  mWindow->window().on_close_requested(
      std::bind(&MainWindow::closeRequested, this));

  // Prepare file system model.
  auto fileSystemModel = std::make_shared<FileSystemModel>(
      mApp.getWorkspace(), mApp.getWorkspace().getProjectsPath(),
      mSettingsPrefix % "/workspace_tree", &mApp.getQuickAccess());
  connect(fileSystemModel.get(), &FileSystemModel::openFileTriggered, &mApp,
          [this](const FilePath& fp) { mApp.openFile(fp, mWidget); });
  connect(fileSystemModel.get(), &FileSystemModel::newProjectTriggered, &mApp,
          [this](const FilePath& parentDir) {
            mApp.createProject(parentDir, false, mWidget);
          });

  // Set global data.
  const ui::Data& d = mWindow->global<ui::Data>();
  d.set_panel_page(ui::PanelPage::Home);
  d.set_workspace_folder_tree(fileSystemModel);
  d.set_notifications_unread(
      mApp.getNotifications().getUnreadNotificationsCount());
  d.set_notifications_progress_index(
      mApp.getNotifications().getCurrentProgressIndex());
  d.set_notifications_shown(false);
  d.set_project_preview_rendering(false);

  // Bind global data to signals.
  connect(&mApp.getNotifications(),
          &NotificationsModel::unreadNotificationsCountChanged, this,
          [this](int count) {
            mWindow->global<ui::Data>().set_notifications_unread(count);
          });
  connect(&mApp.getNotifications(),
          &NotificationsModel::currentProgressIndexChanged, this,
          [this](int index) {
            mWindow->global<ui::Data>().set_notifications_progress_index(index);
          });
  connect(mProjectPreviewRenderer.get(), &ProjectReadmeRenderer::runningChanged,
          this, [this](bool running) {
            mWindow->global<ui::Data>().set_project_preview_rendering(running);
          });
  connect(mProjectPreviewRenderer.get(), &ProjectReadmeRenderer::finished, this,
          [this](const QPixmap& result) {
            mWindow->global<ui::Data>().set_project_preview_image(q2s(result));
          });

  // Register global callbacks.
  const ui::Backend& b = mWindow->global<ui::Backend>();
  b.on_trigger(std::bind(&MainWindow::trigger, this, std::placeholders::_1));
  b.on_request_project_preview(
      [this](const slint::SharedString& fp, float width) {
        mProjectPreviewRenderer->request(FilePath(s2q(fp)),
                                         static_cast<int>(width));
        return true;
      });

  // Update editor command translations & keyboard shortcuts.
  EditorCommandSetUpdater::update(mWindow->global<ui::EditorCommandSet>());
  connect(&mApp.getWorkspace().getSettings().keyboardShortcuts,
          &WorkspaceSettingsItem_KeyboardShortcuts::edited, this, [this]() {
            EditorCommandSetUpdater::update(
                mWindow->global<ui::EditorCommandSet>());
          });

  // Add actions for the keyboard shortcuts.
  EditorCommandSet& cmd = EditorCommandSet::instance();
  auto bind = [this](const EditorCommand& c, ui::Action a) {
    mWidget->addAction(
        c.createAction(mWidget, this, [this, a]() { trigger(a); }));
  };
  bind(cmd.fileManager, ui::Action::WorkspaceOpenFolder);
  bind(cmd.workspaceSwitch, ui::Action::WorkspaceSwitch);
  bind(cmd.workspaceSettings, ui::Action::WorkspaceSettings);
  bind(cmd.workspaceLibrariesRescan, ui::Action::WorkspaceLibrariesRescan);
  bind(cmd.libraryManager, ui::Action::LibraryManager);
  bind(cmd.projectNew, ui::Action::ProjectNew);
  bind(cmd.projectOpen, ui::Action::ProjectOpen);
  bind(cmd.addExampleProjects, ui::Action::ProjectImportExamples);
  bind(cmd.importEagleLibrary, ui::Action::ProjectImportEagle);
  bind(cmd.windowNew, ui::Action::WindowNew);
  bind(cmd.windowClose, ui::Action::WindowClose);
  bind(cmd.applicationQuit, ui::Action::Quit);
  bind(cmd.aboutLibrePcb, ui::Action::AboutDialog);
  bind(cmd.website, ui::Action::Website);
  bind(cmd.documentationOnline, ui::Action::UserManual);
  bind(cmd.support, ui::Action::Support);
  bind(cmd.donate, ui::Action::Donate);
  bind(cmd.keyboardShortcutsReference, ui::Action::KeyboardShortcutsReference);

  // Show window.
  mWindow->show();

  // Load window state.
  QSettings cs;
  mWidget->restoreGeometry(
      cs.value(mSettingsPrefix % "/geometry").toByteArray());
}

MainWindow::~MainWindow() noexcept {
  mWindow->window().on_close_requested(
      []() { return slint::CloseRequestResponse::HideWindow; });
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool MainWindow::isCurrentWindow() const noexcept {
  return mWidget->isActiveWindow();
}

void MainWindow::makeCurrentWindow() noexcept {
  mWidget->show();
  mWidget->raise();
  mWidget->activateWindow();
}

void MainWindow::showPanelPage(ui::PanelPage page) noexcept {
  mWindow->global<ui::Data>().set_panel_page(page);
}

void MainWindow::popUpNotifications() noexcept {
  if (mApp.getNotifications().row_count() > 0) {
    mWindow->global<ui::Data>().set_notifications_shown(true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

slint::CloseRequestResponse MainWindow::closeRequested() noexcept {
  if (!mApp.requestClosingWindow(mWidget)) {
    return slint::CloseRequestResponse::KeepWindowShown;
  }

  // Save window state.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/geometry", mWidget->saveGeometry());

  emit aboutToClose();
  return slint::CloseRequestResponse::HideWindow;
}

bool MainWindow::trigger(ui::Action a) noexcept {
  StandardEditorCommandHandler stdHandler(mApp.getWorkspace().getSettings(),
                                          mWidget);

  switch (a) {
    // General
    case ui::Action::AboutDialog: {
      AboutDialog dlg(mApp.getWorkspace().getSettings(), mWidget);
      dlg.exec();
      return true;
    }
    case ui::Action::LibraryManager: {
      mApp.openLibraryManager();
      return true;
    }
    case ui::Action::KeyboardShortcutsReference: {
      stdHandler.shortcutsReference();
      return true;
    }
    case ui::Action::VideoTutorials: {
      stdHandler.onlineVideoTutorials();
      return true;
    }
    case ui::Action::UserManual: {
      stdHandler.onlineDocumentation();
      return true;
    }
    case ui::Action::Support: {
      stdHandler.onlineSupport();
      return true;
    }
    case ui::Action::Donate: {
      stdHandler.onlineDonate();
      return true;
    }
    case ui::Action::Website: {
      stdHandler.website();
      return true;
    }
    case ui::Action::SourceCode: {
      stdHandler.onlineSourceCode();
      return true;
    }
    case ui::Action::Quit: {
      mApp.quit(mWidget);
      return true;
    }

    // Window
    case ui::Action::WindowNew: {
      mApp.createNewWindow();
      return true;
    }
    case ui::Action::WindowClose: {
      mWindow->hide();
      return true;
    }

    // Workspace
    case ui::Action::WorkspaceOpenFolder: {
      stdHandler.fileManager(mApp.getWorkspace().getPath());
      return true;
    }
    case ui::Action::WorkspaceSwitch: {
      mApp.switchWorkspace(mWidget);
      return true;
    }
    case ui::Action::WorkspaceSettings: {
      mApp.execWorkspaceSettingsDialog(mWidget);
      return true;
    }
    case ui::Action::WorkspaceLibrariesRescan: {
      mApp.getWorkspace().getLibraryDb().startLibraryRescan();
      return true;
    }
    case ui::Action::ProjectImportExamples: {
      mApp.addExampleProjects(mWidget);
      return true;
    }

    // Project
    case ui::Action::ProjectImportEagle: {
      mApp.createProject(FilePath(), true, mWidget);
      return true;
    }
    case ui::Action::ProjectNew: {
      mApp.createProject(FilePath(), false, mWidget);
      return true;
    }
    case ui::Action::ProjectOpen: {
      mApp.openProject(FilePath(), mWidget);
      return true;
    }

    default:
      break;
  }

  qWarning() << "Unhandled UI action:" << static_cast<int>(a);
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
