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

#include "editorcommandsetupdater.h"
#include "guiapplication.h"
#include "library/createlibrarytab.h"
#include "library/downloadlibrarytab.h"
#include "mainwindowtestadapter.h"
#include "notificationsmodel.h"
#include "project/projectreadmerenderer.h"
#include "utils/slinthelpers.h"
#include "utils/standardeditorcommandhandler.h"
#include "windowsection.h"
#include "windowtab.h"
#include "workspace/filesystemmodel.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

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
    mSections(new UiObjectList<WindowSection, ui::WindowSectionData>()),
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
  d.set_sections(mSections);
  d.set_current_section_index(0);
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
  b.on_trigger(
      std::bind(&MainWindow::triggerAsync, this, std::placeholders::_1));
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

  // Setup test adapter.
  connect(mTestAdapter.get(), &MainWindowTestAdapter::actionTriggered, this,
          &MainWindow::triggerAsync);

  // Show window.
  mWindow->show();

  // Load window state.
  QSettings cs;
  mWidget->restoreGeometry(
      cs.value(mSettingsPrefix % "/geometry").toByteArray());
  const int sectionCount = cs.beginReadArray(mSettingsPrefix % "/sections");
  for (int i = 0; i < sectionCount; ++i) {
    splitSection(mSections->count());
  }
  cs.endArray();

  if (mSections->isEmpty()) {
    splitSection(0);
  }
}

MainWindow::~MainWindow() noexcept {
  mWindow->window().on_close_requested(
      []() { return slint::CloseRequestResponse::HideWindow; });
  mWindow->hide();
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
  cs.beginWriteArray(mSettingsPrefix % "/sections", mSections->count());
  cs.endArray();

  emit aboutToClose();
  return slint::CloseRequestResponse::HideWindow;
}

void MainWindow::triggerAsync(ui::Action a) noexcept {
  // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
  QMetaObject::invokeMethod(
      this, [this, a]() { trigger(a); }, Qt::QueuedConnection);
}

bool MainWindow::trigger(ui::Action a) noexcept {
  switch (a) {
    // General
    case ui::Action::LibraryManager: {
      mApp.openLibraryManager();
      return true;
    }
    case ui::Action::KeyboardShortcutsReference: {
      StandardEditorCommandHandler handler(mApp.getWorkspace().getSettings(),
                                           mWidget);
      handler.shortcutsReference();
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
      closeRequested();
      return true;
    }

    // Window section
    case ui::Action::SectionSplit: {
      splitSection(mSections->count() - 1);
      return true;
    }

    // Workspace
    case ui::Action::WorkspaceOpenFolder: {
      StandardEditorCommandHandler handler(mApp.getWorkspace().getSettings(),
                                           mWidget);
      handler.fileManager(mApp.getWorkspace().getPath());
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

    // Library
    case ui::Action::LibraryCreate: {
      if (!switchToTab<CreateLibraryTab>()) {
        addTab(std::make_shared<CreateLibraryTab>(mApp));
      }
      return true;
    }
    case ui::Action::LibraryDownload: {
      if (!switchToTab<DownloadLibraryTab>()) {
        addTab(std::make_shared<DownloadLibraryTab>(mApp));
      }
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

void MainWindow::splitSection(int index) noexcept {
  const int newIndex = qBound(0, index + 1, mSections->count());
  std::shared_ptr<WindowSection> s = std::make_shared<WindowSection>(mApp);
  connect(s.get(), &WindowSection::splitRequested, this, [this]() {
    if (auto index =
            mSections->indexOf(static_cast<const WindowSection*>(sender()))) {
      splitSection(*index);
    }
  });
  connect(s.get(), &WindowSection::closeRequested, this, [this]() {
    if (mSections->count() > 1) {
      if (std::shared_ptr<WindowSection> s =
              mSections->take(static_cast<const WindowSection*>(sender()))) {
        const ui::Data& d = mWindow->global<ui::Data>();
        d.set_current_section_index(
            qBound(-1, d.get_current_section_index(), mSections->count() - 1));
        updateHomeTabSection();
      }
    }
  });
  connect(s.get(), &WindowSection::statusBarMessageChanged, this,
          [this](const QString& message, int timeoutMs) {
            const ui::Data& d = mWindow->global<ui::Data>();
            d.set_status_bar_message(q2s(message));
            if (timeoutMs > 0) {
              QTimer::singleShot(timeoutMs, this, [&d, message]() {
                if (s2q(d.get_status_bar_message()) == message) {
                  d.set_status_bar_message(slint::SharedString());
                }
              });
            }
          });
  mSections->insert(newIndex, s);
  updateHomeTabSection();
}

void MainWindow::updateHomeTabSection() noexcept {
  for (int i = 0; i < mSections->count(); ++i) {
    mSections->at(i)->setHomeTabVisible(i == 0);
  }
}

void MainWindow::addTab(std::shared_ptr<WindowTab> tab) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  const int sectionIndex =
      qBound(0, d.get_current_section_index(), mSections->count() - 1);
  if (std::shared_ptr<WindowSection> s = mSections->value(sectionIndex)) {
    s->addTab(tab);
  }
}

template <typename T>
bool MainWindow::switchToTab() noexcept {
  for (auto section : *mSections) {
    if (section->switchToTab<T>()) {
      return true;
    }
  }

  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
