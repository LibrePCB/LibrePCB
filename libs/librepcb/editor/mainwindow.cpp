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
#include "library/librariesmodel.h"
#include "mainwindowtestadapter.h"
#include "notificationsmodel.h"
#include "project/board/board2dtab.h"
#include "project/board/board3dtab.h"
#include "project/newprojectwizard/newprojectwizard.h"
#include "project/outputjobsdialog/outputjobsdialog.h"
#include "project/projecteditor2.h"
#include "project/projectreadmerenderer.h"
#include "project/projectsmodel.h"
#include "project/schematic/schematictab.h"
#include "utils/slinthelpers.h"
#include "utils/standardeditorcommandhandler.h"
#include "windowsection.h"
#include "windowtab.h"
#include "workspace/filesystemmodel.h"
#include "workspace/initializeworkspacewizard/initializeworkspacewizard.h"

#include <librepcb/core/project/project.h>
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
  // connect(mSections.get(), &WindowSectionsModel::currentProjectChanged, this,
  //         &MainWindow::setCurrentProject);
  // connect(mSections.get(), &WindowSectionsModel::cursorCoordinatesChanged,
  // this,
  //         [&d](const Point& pos, const LengthUnit& unit) {
  //           d.set_cursor_coordinates(
  //               q2s(QString("X: %1 Y: %2")
  //                       .arg(unit.convertToUnit(pos.getX()), 10, 'f',
  //                            unit.getReasonableNumberOfDecimals())
  //                       .arg(unit.convertToUnit(pos.getY()), 10, 'f',
  //                            unit.getReasonableNumberOfDecimals())));
  //         });
  // connect(mSections.get(), &WindowSectionsModel::statusBarMessageChanged,
  // this,
  //         [this, &d](const QString& message, int timeoutMs) {
  //           d.set_status_bar_message(q2s(message));
  //           if (timeoutMs > 0) {
  //             QTimer::singleShot(timeoutMs, this, [&d, message]() {
  //               if (s2q(d.get_status_bar_message()) == message) {
  //                 d.set_status_bar_message(slint::SharedString());
  //               }
  //             });
  //           }
  //         });
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
  b.on_schematic_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      addTab(std::make_shared<SchematicTab>(mApp, prj, index));
    }
  });
  b.on_board_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      auto tab = std::make_shared<Board2dTab>(mApp, prj, index);
      connect(tab.get(), &Board2dTab::board3dRequested, this,
              [this](std::shared_ptr<ProjectEditor2> prj, int boardIndex) {
                addTab(std::make_shared<Board3dTab>(mApp, prj, boardIndex));
              });
      addTab(tab);
    }
  });
  b.on_render_scene(
      [this](int sectionIndex, float width, float height, int frameIndex) {
        Q_UNUSED(frameIndex);
        if (auto section = mSections->value(sectionIndex)) {
          return section->renderScene(width, height);
        } else {
          return slint::Image();
        }
      });
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  const slint::Point<float>& scenePos,
                                  slint::private_api::PointerEvent e) {
    bool handled = false;
    if (auto section = mSections->value(sectionIndex)) {
      // const auto winPos = mWindow->window().position(); NOT WORKING
      QPointF globalPos(scenePos.x + x, scenePos.y + y);
      if (QWidget* win = qApp->activeWindow()) {
        globalPos = win->mapToGlobal(globalPos);
      }
      handled = section->processScenePointerEvent(QPointF(x, y), globalPos, e);
    }
    return handled ? slint::private_api::EventResult::Accept
                   : slint::private_api::EventResult::Reject;
  });
  b.on_scene_scrolled([this](int sectionIndex, float x, float y,
                             slint::private_api::PointerScrollEvent e) {
    bool handled = false;
    if (auto section = mSections->value(sectionIndex)) {
      handled = section->processSceneScrolled(x, y, e);
    }
    return handled ? slint::private_api::EventResult::Accept
                   : slint::private_api::EventResult::Reject;
  });
  b.on_scene_key_pressed(
      [this](int sectionIndex, const slint::private_api::KeyEvent& e) {
        bool handled = false;
        if (auto section = mSections->value(sectionIndex)) {
          handled = section->processSceneKeyPressed(e);
        }
        return handled ? slint::private_api::EventResult::Accept
                       : slint::private_api::EventResult::Reject;
      });
  b.on_scene_key_released(
      [this](int sectionIndex, const slint::private_api::KeyEvent& e) {
        bool handled = false;
        if (auto section = mSections->value(sectionIndex)) {
          handled = section->processSceneKeyReleased(e);
        }
        return handled ? slint::private_api::EventResult::Accept
                       : slint::private_api::EventResult::Reject;
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
      [this](ui::Action a) { trigger(a); }, Qt::QueuedConnection);

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

void MainWindow::closeProject(int index,
                              std::shared_ptr<ProjectEditor2> prj) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  if (d.get_current_project_index() >= index) {
    d.set_current_project_index(
        std::min(index, static_cast<int>(mApp.getProjects().row_count()) - 2));
  }
  // mSections->closeProjectTabs(prj);
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
    case ui::Action::LibraryPanelEnsurePopulated: {
      mApp.getLocalLibraries().ensurePopulated(true);
      mApp.getRemoteLibraries().ensurePopulated(true);
      return true;
    }
    case ui::Action::LibraryPanelApply: {
      mApp.getRemoteLibraries().applyChanges();
      return true;
    }
    case ui::Action::LibraryPanelCancel: {
      mApp.getRemoteLibraries().cancel();
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

    default:
      break;
  }

  /*if (a == ui::Action::ProjectOpenOutputJobs) {
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
  }*/

  qWarning() << "Unhandled UI action:" << static_cast<int>(a);
  return false;
}

void MainWindow::splitSection(int index) noexcept {
  const int newIndex = qBound(0, index + 1, mSections->count());
  std::shared_ptr<WindowSection> s = std::make_shared<WindowSection>(mApp);
  connect(s.get(), &WindowSection::panelPageRequested, this,
          &MainWindow::showPanelPage);
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
