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
#include "project/board/boardeditor.h"
#include "project/projecteditor.h"
#include "project/projectreadmerenderer.h"
#include "project/schematic/schematiceditor.h"
#include "project/schematic/schematictab.h"
#include "utils/slinthelpers.h"
#include "utils/standardeditorcommandhandler.h"
#include "windowsection.h"
#include "windowtab.h"
#include "workspace/filesystemmodel.h"

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
    mTestAdapter(new MainWindowTestAdapter(app, *this, mWidget)) {
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
  b.on_trigger([this](ui::Action a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, a]() { trigger(a); }, Qt::QueuedConnection);
  });
  b.on_trigger_section([this](int section, ui::WindowSectionAction a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, section, a]() { triggerSection(section, a); },
        Qt::QueuedConnection);
  });
  b.on_trigger_tab([this](int section, int tab, ui::TabAction a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, section, tab, a]() { triggerTab(section, tab, a); },
        Qt::QueuedConnection);
  });
  b.on_trigger_project([this](int index, ui::ProjectAction a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, index, a]() { triggerProject(index, a); },
        Qt::QueuedConnection);
  });
  b.on_trigger_schematic(
      [this](int project, int schematic, ui::SchematicAction a) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, project, schematic, a]() {
              triggerSchematic(project, schematic, a);
            },
            Qt::QueuedConnection);
      });
  b.on_trigger_board([this](int project, int board, ui::BoardAction a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, project, board, a]() { triggerBoard(project, board, a); },
        Qt::QueuedConnection);
  });
  b.on_render_scene([this](int sectionIndex, float width, float height,
                           int scene, int frameIndex) {
    Q_UNUSED(frameIndex);
    if (auto section = mSections->value(sectionIndex)) {
      return section->renderScene(width, height, scene);
    } else {
      return slint::Image();
    }
  });
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  slint::private_api::PointerEvent e) {
    if (auto section = mSections->value(sectionIndex)) {
      section->processScenePointerEvent(QPointF(x, y), e);
    }
  });
  b.on_scene_scrolled([this](int sectionIndex, float x, float y,
                             slint::private_api::PointerScrollEvent e) {
    bool handled = false;
    if (auto section = mSections->value(sectionIndex)) {
      handled = section->processSceneScrolled(QPointF(x, y), e);
    }
    return handled;
  });
  b.on_scene_key_event(
      [this](int sectionIndex, const slint::private_api::KeyEvent& e) {
        bool handled = false;
        if (auto section = mSections->value(sectionIndex)) {
          handled = section->processSceneKeyEvent(e);
        }
        return handled;
      });
  b.on_request_project_preview(
      [this](const slint::SharedString& fp, float width) {
        mProjectPreviewRenderer->request(FilePath(s2q(fp)),
                                         static_cast<int>(width));
        return true;
      });

  // Update UI state.
  d.fn_current_tab_changed();

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
  d.set_rule_check_zoom_to_location(
      cs.value(mSettingsPrefix % "/rule_check_zoom_to_location", true)
          .toBool());
  d.set_order_pcb_open_web_browser(
      cs.value(mSettingsPrefix % "/order_open_web_browser", true).toBool());
  const int sectionCount = cs.beginReadArray(mSettingsPrefix % "/sections");
  for (int i = 0; i < sectionCount; ++i) {
    splitSection(mSections->count(), false);
  }
  cs.endArray();

  if (mSections->isEmpty()) {
    splitSection(0, true);
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

void MainWindow::showStatusBarMessage(const QString& message, int timeoutMs) {
  const ui::Data& d = mWindow->global<ui::Data>();
  d.set_status_bar_message(q2s(message));

  if (timeoutMs > 0) {
    QTimer::singleShot(timeoutMs, this, [&d, message]() {
      if (s2q(d.get_status_bar_message()) == message) {
        d.set_status_bar_message(slint::SharedString());
      }
    });
  }
}

void MainWindow::setCurrentProject(int index) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  d.fn_set_current_project(index);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

slint::CloseRequestResponse MainWindow::closeRequested() noexcept {
  if (!mApp.requestClosingWindow()) {
    return slint::CloseRequestResponse::KeepWindowShown;
  }

  // Save window state.
  QSettings cs;
  const ui::Data& d = mWindow->global<ui::Data>();
  cs.setValue(mSettingsPrefix % "/geometry", mWidget->saveGeometry());
  cs.setValue(mSettingsPrefix % "/rule_check_zoom_to_location",
              d.get_rule_check_zoom_to_location());
  cs.setValue(mSettingsPrefix % "/order_open_web_browser",
              d.get_order_pcb_open_web_browser());
  cs.beginWriteArray(mSettingsPrefix % "/sections", mSections->count());
  cs.endArray();

  emit aboutToClose();
  return slint::CloseRequestResponse::HideWindow;
}

void MainWindow::trigger(ui::Action a) noexcept {
  switch (a) {
    // General
    case ui::Action::KeyboardShortcutsReference: {
      StandardEditorCommandHandler handler(mApp.getWorkspace().getSettings(),
                                           mWidget);
      handler.shortcutsReference();
      break;
    }
    case ui::Action::Quit: {
      mApp.quit(mWidget);
      break;
    }

    // Window
    case ui::Action::WindowNew: {
      mApp.createNewWindow();
      break;
    }
    case ui::Action::WindowClose: {
      closeRequested();
      break;
    }

    // Workspace
    case ui::Action::WorkspaceOpenFolder: {
      StandardEditorCommandHandler handler(mApp.getWorkspace().getSettings(),
                                           mWidget);
      handler.fileManager(mApp.getWorkspace().getPath());
      break;
    }
    case ui::Action::WorkspaceSwitch: {
      mApp.switchWorkspace(mWidget);
      break;
    }
    case ui::Action::WorkspaceSettings: {
      mApp.execWorkspaceSettingsDialog(mWidget);
      break;
    }
    case ui::Action::WorkspaceLibrariesRescan: {
      mApp.getWorkspace().getLibraryDb().startLibraryRescan();
      break;
    }
    case ui::Action::ProjectImportExamples: {
      mApp.addExampleProjects(mWidget);
      break;
    }

    // Library
    case ui::Action::LibraryCreate: {
      if (!switchToTab<CreateLibraryTab>()) {
        addTab(std::make_shared<CreateLibraryTab>(mApp));
      }
      break;
    }
    case ui::Action::LibraryDownload: {
      if (!switchToTab<DownloadLibraryTab>()) {
        addTab(std::make_shared<DownloadLibraryTab>(mApp));
      }
      break;
    }
    case ui::Action::LibraryPanelEnsurePopulated: {
      mApp.getLocalLibraries().ensurePopulated(true);
      mApp.getRemoteLibraries().ensurePopulated(true);
      break;
    }
    case ui::Action::LibraryPanelApply: {
      mApp.getRemoteLibraries().applyChanges();
      break;
    }
    case ui::Action::LibraryPanelCancel: {
      mApp.getRemoteLibraries().cancel();
      break;
    }

    // Project
    case ui::Action::ProjectImportEagle: {
      mApp.createProject(FilePath(), true, mWidget);
      break;
    }
    case ui::Action::ProjectNew: {
      mApp.createProject(FilePath(), false, mWidget);
      break;
    }
    case ui::Action::ProjectOpen: {
      mApp.openProject(FilePath(), mWidget);
      break;
    }

    default: {
      qWarning() << "Unhandled UI action:" << static_cast<int>(a);
      break;
    }
  }
}

void MainWindow::triggerSection(int section,
                                ui::WindowSectionAction a) noexcept {
  switch (a) {
    case ui::WindowSectionAction::Split: {
      splitSection(section, true);
      break;
    }
    case ui::WindowSectionAction::Close: {
      if (mSections->count() > 1) {
        if (std::shared_ptr<WindowSection> s = mSections->takeAt(section)) {
          const ui::Data& d = mWindow->global<ui::Data>();
          d.set_current_section_index(qBound(-1, d.get_current_section_index(),
                                             mSections->count() - 1));
          updateHomeTabSection();
          d.fn_current_tab_changed();
        }
      }
      break;
    }
    default: {
      qWarning() << "Unhandled section action:" << static_cast<int>(a);
      break;
    }
  }
}

void MainWindow::triggerTab(int section, int tab, ui::TabAction a) noexcept {
  if (auto s = mSections->value(section)) {
    s->triggerTab(tab, a);
  }
}

void MainWindow::triggerProject(int index, ui::ProjectAction a) noexcept {
  std::shared_ptr<ProjectEditor> editor = mApp.getProjects().value(index);
  if (!editor) return;

  switch (a) {
    case ui::ProjectAction::Close: {
      if (editor->requestClose()) {
        mApp.closeProject(index);
      }
      break;
    }
    case ui::ProjectAction::NewSheet: {
      if (auto schEditor = editor->execNewSheetDialog()) {
        openSchematicTab(index, schEditor->getUiIndex());
      }
      break;
    }
    case ui::ProjectAction::NewBoard: {
      if (auto brdEditor = editor->execNewBoardDialog(std::nullopt)) {
        openBoard2dTab(index, brdEditor->getUiIndex());
      }
      break;
    }
    default: {
      editor->trigger(a);
      break;
    }
  }
}

void MainWindow::triggerSchematic(int project, int schematic,
                                  ui::SchematicAction a) noexcept {
  std::shared_ptr<ProjectEditor> prjEditor = mApp.getProjects().value(project);
  if (!prjEditor) return;

  switch (a) {
    case ui::SchematicAction::Open: {
      openSchematicTab(project, schematic);
      break;
    }
    case ui::SchematicAction::Rename: {
      prjEditor->execRenameSheetDialog(schematic);
      break;
    }
    case ui::SchematicAction::Delete: {
      prjEditor->execDeleteSheetDialog(schematic);
      break;
    }
    default: {
      qWarning() << "Unhandled action in MainWindow::triggerSchematic():"
                 << static_cast<int>(a);
      break;
    }
  }
}

void MainWindow::triggerBoard(int project, int board,
                              ui::BoardAction a) noexcept {
  std::shared_ptr<ProjectEditor> prjEditor = mApp.getProjects().value(project);
  if (!prjEditor) return;

  switch (a) {
    case ui::BoardAction::Open2d: {
      openBoard2dTab(project, board);
      break;
    }
    case ui::BoardAction::Open3d: {
      openBoard3dTab(project, board);
      break;
    }
    case ui::BoardAction::Copy: {
      if (auto brdEditor = prjEditor->execNewBoardDialog(board)) {
        openBoard2dTab(project, brdEditor->getUiIndex());
      }
      break;
    }
    case ui::BoardAction::Delete: {
      prjEditor->execDeleteBoardDialog(board);
      break;
    }
    case ui::BoardAction::ExportStep: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->execStepExportDialog();
      }
      break;
    }
    case ui::BoardAction::RunQuickCheck: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->startDrc(true);
      }
      break;
    }
    case ui::BoardAction::RunDrc: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->startDrc(false);
      }
      break;
    }
    case ui::BoardAction::OpenSetupDialog: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->execBoardSetupDialog();
      }
      break;
    }
    case ui::BoardAction::OpenDrcSetupDialog: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->execBoardSetupDialog(true);
      }
      break;
    }
    case ui::BoardAction::PrepareOrder: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        brdEditor->prepareOrderPcb();
      }
      break;
    }
    case ui::BoardAction::StartOrder: {
      if (auto brdEditor = prjEditor->getBoards().value(board)) {
        const ui::Data& d = mWindow->global<ui::Data>();
        brdEditor->startOrderPcbUpload(d.get_order_pcb_open_web_browser());
      }
      break;
    }
    default: {
      qWarning() << "Unhandled action in MainWindow::triggerBoard():"
                 << static_cast<int>(a);
      break;
    }
  }
}

void MainWindow::openSchematicTab(int projectIndex, int index) noexcept {
  if (!switchToProjectTab<SchematicTab>(projectIndex, index)) {
    if (auto prjEditor = mApp.getProjects().value(projectIndex)) {
      if (auto schEditor = prjEditor->getSchematics().value(index)) {
        addTab(std::make_shared<SchematicTab>(mApp, *schEditor));
      }
    }
  }
}

void MainWindow::openBoard2dTab(int projectIndex, int index) noexcept {
  if (!switchToProjectTab<Board2dTab>(projectIndex, index)) {
    if (auto prjEditor = mApp.getProjects().value(projectIndex)) {
      if (auto brdEditor = prjEditor->getBoards().value(index)) {
        addTab(std::make_shared<Board2dTab>(mApp, *brdEditor));
      }
    }
  }
}

void MainWindow::openBoard3dTab(int projectIndex, int index) noexcept {
  if (!switchToProjectTab<Board3dTab>(projectIndex, index)) {
    if (auto prjEditor = mApp.getProjects().value(projectIndex)) {
      if (auto brdEditor = prjEditor->getBoards().value(index)) {
        addTab(std::make_shared<Board3dTab>(mApp, *brdEditor));
      }
    }
  }
}

void MainWindow::splitSection(int index, bool makeCurrent) noexcept {
  const int newIndex = qBound(0, index + 1, mSections->count());
  std::shared_ptr<WindowSection> s = std::make_shared<WindowSection>(mApp);
  connect(s.get(), &WindowSection::currentTabChanged, this, [this]() {
    const ui::Data& d = mWindow->global<ui::Data>();
    d.fn_current_tab_changed();
  });
  connect(s.get(), &WindowSection::panelPageRequested, this,
          &MainWindow::showPanelPage);
  connect(s.get(), &WindowSection::cursorCoordinatesChanged, this,
          [this](const Point& pos, const LengthUnit& unit) {
            const ui::Data& d = mWindow->global<ui::Data>();
            d.set_cursor_coordinates(
                q2s(QString("%1, %2")
                        .arg(unit.convertToUnit(pos.getX()), 1, 'f',
                             unit.getReasonableNumberOfDecimals())
                        .arg(unit.convertToUnit(pos.getY()), 1, 'f',
                             unit.getReasonableNumberOfDecimals())));
          });
  connect(s.get(), &WindowSection::statusBarMessageChanged, this,
          &MainWindow::showStatusBarMessage);
  mSections->insert(newIndex, s);

  if (makeCurrent || (mSections->count() == 1)) {
    const ui::Data& d = mWindow->global<ui::Data>();
    d.set_current_section_index(newIndex);
    d.fn_current_tab_changed();
  }

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

template <typename T>
bool MainWindow::switchToProjectTab(int prjIndex, int objIndex) noexcept {
  for (auto section : *mSections) {
    if (section->switchToProjectTab<T>(prjIndex, objIndex)) {
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
