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
#include "library/librariesmodel.h"
#include "mainwindowtestadapter.h"
#include "notificationsmodel.h"
#include "project/newprojectwizard/newprojectwizard.h"
#include "project/outputjobsdialog/outputjobsdialog.h"
#include "project/projecteditor2.h"
#include "project/projectreadmerenderer.h"
#include "project/projectsmodel.h"
#include "utils/slinthelpers.h"
#include "utils/standardeditorcommandhandler.h"
#include "windowsectionsmodel.h"
#include "workspace/desktopservices.h"
#include "workspace/filesystemmodel.h"
#include "workspace/initializeworkspacewizard/initializeworkspacewizard.h"

#include <librepcb/core/project/project.h>
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
    mSections(
        new WindowSectionsModel(app, win->global<ui::Data>(), mSettingsPrefix)),
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
  d.set_cursor_coordinates(slint::SharedString());
  d.set_ignore_placement_locks(false);
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
  connect(mSections.get(), &WindowSectionsModel::currentProjectChanged, this,
          &MainWindow::setCurrentProject);
  connect(mSections.get(), &WindowSectionsModel::cursorCoordinatesChanged, this,
          [&d](const Point& pos, const LengthUnit& unit) {
            d.set_cursor_coordinates(
                q2s(QString("X: %1 Y: %2")
                        .arg(unit.convertToUnit(pos.getX()), 10, 'f',
                             unit.getReasonableNumberOfDecimals())
                        .arg(unit.convertToUnit(pos.getY()), 10, 'f',
                             unit.getReasonableNumberOfDecimals())));
          });
  connect(mSections.get(), &WindowSectionsModel::statusBarMessageChanged, this,
          [this, &d](const QString& message, int timeoutMs) {
            d.set_status_bar_message(q2s(message));
            if (timeoutMs > 0) {
              QTimer::singleShot(timeoutMs, this, [&d, message]() {
                if (s2q(d.get_status_bar_message()) == message) {
                  d.set_status_bar_message(slint::SharedString());
                }
              });
            }
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
  b.on_trigger(std::bind(&MainWindow::triggerAsync, this, std::placeholders::_1,
                         std::placeholders::_2));
  b.on_schematic_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      mSections->openSchematic(prj, index);
    }
  });
  b.on_board_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      mSections->openBoard(prj, index);
    }
  });
  b.on_tab_clicked(std::bind(&WindowSectionsModel::setCurrentTab,
                             mSections.get(), std::placeholders::_1,
                             std::placeholders::_2));
  b.on_tab_close_clicked(std::bind(&WindowSectionsModel::closeTab,
                                   mSections.get(), std::placeholders::_1,
                                   std::placeholders::_2));
  b.on_render_scene(std::bind(
      &WindowSectionsModel::renderScene, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  const slint::Point<float>& scenePos,
                                  slint::private_api::PointerEvent e) {
    // const auto winPos = mWindow->window().position(); NOT WORKING
    QPointF globalPos(scenePos.x + x, scenePos.y + y);
    if (QWidget* win = qApp->activeWindow()) {
      globalPos = win->mapToGlobal(globalPos);
    }
    return mSections->processScenePointerEvent(sectionIndex, QPointF(x, y),
                                               globalPos, e);
  });
  b.on_scene_scrolled(std::bind(&WindowSectionsModel::processSceneScrolled,
                                mSections.get(), std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3,
                                std::placeholders::_4));
  b.on_scene_key_pressed(std::bind(&WindowSectionsModel::processSceneKeyPressed,
                                   mSections.get(), std::placeholders::_1,
                                   std::placeholders::_2));
  b.on_scene_key_released(
      std::bind(&WindowSectionsModel::processSceneKeyReleased, mSections.get(),
                std::placeholders::_1, std::placeholders::_2));
  b.on_scene_zoom_fit_clicked(std::bind(
      &WindowSectionsModel::zoomFit, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_in_clicked(std::bind(
      &WindowSectionsModel::zoomIn, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_out_clicked(std::bind(
      &WindowSectionsModel::zoomOut, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
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
  connect(
      mTestAdapter.get(), &MainWindowTestAdapter::actionTriggered, this,
      [this](ui::Action a) { trigger(a, -1); }, Qt::QueuedConnection);
  connect(
      mTestAdapter.get(), &MainWindowTestAdapter::panelPageTriggered, this,
      [this](ui::PanelPage p) { showPanelPage(p); }, Qt::QueuedConnection);

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

void MainWindow::closeProject(int index,
                              std::shared_ptr<ProjectEditor2> prj) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  if (d.get_current_project_index() >= index) {
    d.set_current_project_index(
        std::min(index, static_cast<int>(mApp.getProjects().row_count()) - 2));
  }
  mSections->closeProjectTabs(prj);
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

void MainWindow::triggerAsync(ui::Action a, int sectionIndex) noexcept {
  // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
  QMetaObject::invokeMethod(
      this, [this, a, sectionIndex]() { trigger(a, sectionIndex); },
      Qt::QueuedConnection);
}

bool MainWindow::trigger(ui::Action a, int sectionIndex) noexcept {
  if (mSections->trigger(a, sectionIndex)) {
    return true;
  }

  switch (a) {
    // General
    case ui::Action::KeyboardShortcutsReference: {
      StandardEditorCommandHandler handler(mApp.getWorkspace().getSettings(),
                                           mWidget);
      handler.shortcutsReference();
      return true;
    }
    case ui::Action::CopyApplicationDetailsIntoClipboard: {
      QApplication::clipboard()->setText(
          s2q(mWindow->global<ui::Data>().get_about_librepcb_details()));
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

    // Project
    case ui::Action::ProjectImportEagle: {
      newProject(true);
      return true;
    }
    case ui::Action::ProjectNew: {
      newProject();
      return true;
    }
    case ui::Action::ProjectOpen: {
      setCurrentProject(mApp.getProjects().openProject());
      return true;
    }

    // Library panel
    case ui::Action::LibraryPanelEnsurePopulated: {
      mApp.getLibraries().ensurePopulated();
      return true;
    }
    case ui::Action::LibraryPanelInstall: {
      mApp.getLibraries().installCheckedLibraries();
      return true;
    }

    default:
      break;
  }

  if (a == ui::Action::ProjectOpenOutputJobs) {
    if (std::shared_ptr<ProjectEditor2> editor =
            mApp.getProjects().getProject(sectionIndex)) {
      OutputJobsDialog dlg(mApp.getWorkspace().getSettings(),
                           editor->getProject(), editor->getUndoStack(),
                           mSettingsPrefix % "/output_jobs_dialog", mWidget);
      dlg.exec();
      return true;
    }
  } else if (a == ui::Action::CopyApplicationDetailsIntoClipboard) {
    QApplication::clipboard()->setText(
        s2q(mWindow->global<ui::Data>().get_about_librepcb_details()));
    return true;
  }

  qWarning() << "Unhandled UI action:" << static_cast<int>(a);
  return false;
}

void MainWindow::openFile(const FilePath& fp) noexcept {
  if ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz")) {
    setCurrentProject(mApp.getProjects().openProject(fp));
    showPanelPage(ui::PanelPage::Project);
  } else if (fp.isValid()) {
    DesktopServices ds(mApp.getWorkspace().getSettings());
    ds.openLocalPath(fp);
  }
}

void MainWindow::setCurrentProject(
    std::shared_ptr<ProjectEditor2> prj) noexcept {
  if (prj) {
    mWindow->global<ui::Data>().set_current_project_index(
        mApp.getProjects().getIndexOf(prj));
  }
}

std::shared_ptr<ProjectEditor2> MainWindow::getCurrentProjectEditor() noexcept {
  return mApp.getProjects().getProject(
      mWindow->global<ui::Data>().get_current_project_index());
}

void MainWindow::newProject(bool eagleImport,
                            const FilePath& parentDir) noexcept {
  const NewProjectWizard::Mode mode = eagleImport
      ? NewProjectWizard::Mode::EagleImport
      : NewProjectWizard::Mode::NewProject;
  NewProjectWizard wizard(mApp.getWorkspace(), mode, qApp->activeWindow());
  if (parentDir.isValid()) {
    wizard.setLocationOverride(parentDir);
  }
  if (wizard.exec() == QWizard::Accepted) {
    try {
      std::unique_ptr<Project> project = wizard.createProject();  // can throw
      const FilePath fp = project->getFilepath();
      project.reset();  // Release lock.
      setCurrentProject(mApp.getProjects().openProject(fp));
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(),
                            tr("Could not create project"), e.getMsg());
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
