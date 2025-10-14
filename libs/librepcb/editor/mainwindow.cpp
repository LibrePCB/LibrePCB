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

#include "dialogs/directorylockhandlerdialog.h"
#include "editorcommandsetupdater.h"
#include "guiapplication.h"
#include "library/cat/componentcategorytab.h"
#include "library/cat/packagecategorytab.h"
#include "library/cmp/componenttab.h"
#include "library/createlibrarytab.h"
#include "library/dev/devicetab.h"
#include "library/downloadlibrarytab.h"
#include "library/eaglelibraryimportwizard/eaglelibraryimportwizard.h"
#include "library/kicadlibraryimportwizard/kicadlibraryimportwizard.h"
#include "library/lib/librarytab.h"
#include "library/librariesmodel.h"
#include "library/libraryeditor.h"
#include "library/pkg/packagetab.h"
#include "library/sym/symboltab.h"
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
#include "workspace/desktopservices.h"
#include "workspace/filesystemmodel.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/erc/electricalrulecheckmessages.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

// Detect window size enforced by environment variable (required for testing).
static std::optional<QSize> getOverrideWindowSize() noexcept {
  static const QStringList numbers =
      QString(qgetenv("LIBREPCB_WINDOW_SIZE")).split("x");
  const int width = numbers.value(0).toInt();
  const int height = numbers.value(1).toInt();
  if ((width > 0) && (height > 0)) {
    return QSize(width, height);
  } else {
    return std::nullopt;
  }
}

static bool askForRestoringBackup(const FilePath&) {
  QMessageBox::StandardButton btn = QMessageBox::question(
      qApp->activeWindow(), MainWindow::tr("Restore autosave backup?"),
      MainWindow::tr(
          "It seems that the application crashed the last time you opened "
          "this library element. Do you want to restore the last autosave "
          "backup?"),
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

static LocalizedNameMap copyLibraryElementNames(const LocalizedNameMap& names) {
  // Note: We copy only the default locale for now because the UI doesn't show
  // the other locales so the user can't edit them.
  QString newNameStr = *names.getDefaultValue() % " (" %
      MainWindow::tr("Copy", "The noun (a copy of), not the verb (to copy)") %
      ")";
  if (auto newName = parseElementName(newNameStr)) {
    return LocalizedNameMap(*newName);
  }
  newNameStr = *names.getDefaultValue() % " (Copy)";
  if (auto newName = parseElementName(newNameStr)) {
    return LocalizedNameMap(*newName);
  }
  return LocalizedNameMap(names.getDefaultValue());
}

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
  b.on_trigger_library([this](slint::SharedString path, ui::LibraryAction a) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, path, a]() { triggerLibrary(path, a); },
        Qt::QueuedConnection);
  });
  b.on_trigger_library_element(
      [this](slint::SharedString path, ui::LibraryElementAction a) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this, [this, path, a]() { triggerLibraryElement(path, a); },
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
  if (auto size = getOverrideWindowSize()) {
    qInfo() << "Window size enforced to" << *size;
    mWidget->resize(*size);
  } else if (!mWidget->restoreGeometry(
                 cs.value(mSettingsPrefix % "/geometry").toByteArray())) {
    // By default, open the window maximized as this is more intuitive than a
    // small window with hardcoded, screen-independent size in the Slint file
    // (https://github.com/LibrePCB/LibrePCB/issues/355).
    qInfo() << "Could not restore window geometry, thus maximizing.";
    mWidget->setWindowState(Qt::WindowMaximized | Qt::WindowActive);
  }
  d.set_erc_zoom_to_location(
      cs.value(mSettingsPrefix % "/erc_zoom_to_location", true).toBool());
  d.set_drc_zoom_to_location(
      cs.value(mSettingsPrefix % "/drc_zoom_to_location", true).toBool());
  d.set_order_pcb_open_web_browser(
      cs.value(mSettingsPrefix % "/order_open_web_browser", true).toBool());
  const int sectionCount = cs.beginReadArray(mSettingsPrefix % "/sections");
  for (int i = 0; i < sectionCount; ++i) {
    addSection(mSections->count(), false);
  }
  cs.endArray();

  if (mSections->isEmpty()) {
    addSection(0, true);
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

void MainWindow::addSection(int newIndex, bool makeCurrent) noexcept {
  newIndex = qBound(0, newIndex, mSections->count());
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

void MainWindow::addTab(std::shared_ptr<WindowTab> tab, int section, int index,
                        bool switchToTab, bool switchToSection) noexcept {
  // If no section was specified, add it to the end.
  const ui::Data& d = mWindow->global<ui::Data>();
  if (section < 0) {
    section = d.get_current_section_index();
  }

  // Bound the section to make sure the tab is not lost on invalid indices.
  section = qBound(0, section, mSections->count() - 1);

  if (auto s = mSections->value(section)) {
    s->addTab(tab, index, switchToTab);
    if (switchToSection) {
      d.set_current_section_index(section);
      d.fn_current_tab_changed();
    }
  }
}

std::shared_ptr<WindowTab> MainWindow::removeTab(
    int section, int tab, bool* wasCurrentTab,
    bool* wasCurrentSection) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  const int currentSection = d.get_current_section_index();
  if (wasCurrentSection) {
    *wasCurrentSection = (section == currentSection);
  }
  if (auto s = mSections->value(section)) {
    return s->removeTab(tab, wasCurrentTab);
  }
  return nullptr;
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

void MainWindow::highlightErcMessage(
    ProjectEditor& prjEditor, std::shared_ptr<const RuleCheckMessage> msg,
    bool zoomTo) noexcept {
  auto ercMsg = std::dynamic_pointer_cast<const ErcMsgBase>(msg);
  if (!ercMsg) {
    qCritical() << "ERC message is not derived from ErcMsgBase.";
    return;
  }
  std::optional<int> schIndex;
  for (int i = 0; i < prjEditor.getSchematics().count(); ++i) {
    auto se = prjEditor.getSchematics().at(i);
    if (ercMsg->getSchematic() == se->getSchematic().getUuid()) {
      schIndex = i;
      break;
    }
  }
  if (!schIndex) {
    qCritical() << "ERC message from unknown schematic.";
    return;
  }
  if (auto tab = openSchematicTab(prjEditor.getUiIndex(), *schIndex)) {
    tab->highlightErcMessage(ercMsg, zoomTo);
  }
}

void MainWindow::setCurrentLibrary(int index) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  d.fn_set_current_library(index);
}

void MainWindow::setCurrentProject(int index) noexcept {
  const ui::Data& d = mWindow->global<ui::Data>();
  d.fn_set_current_project(index);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

slint::CloseRequestResponse MainWindow::closeRequested() noexcept {
  // Any message boxes might delay closing the window so we don't want to
  // autosave any intermediate window state during this process. The timer
  // will be restarted by GuiApplication once the window was closed.
  mApp.stopWindowStateAutosaveTimer();

  // Ask to close tabs, projects, libraries etc.
  if (mApp.getWindowCount() >= 2) {
    for (auto section : *mSections) {
      if (!section->requestCloseAllTabs()) {
        return slint::CloseRequestResponse::KeepWindowShown;
      }
    }
  } else if ((!mApp.requestClosingAllProjects()) ||
             (!mApp.requestClosingAllLibraries())) {
    return slint::CloseRequestResponse::KeepWindowShown;
  }

  // Save window state.
  QSettings cs;
  const ui::Data& d = mWindow->global<ui::Data>();
  cs.setValue(mSettingsPrefix % "/geometry", mWidget->saveGeometry());
  cs.setValue(mSettingsPrefix % "/erc_zoom_to_location",
              d.get_erc_zoom_to_location());
  cs.setValue(mSettingsPrefix % "/drc_zoom_to_location",
              d.get_drc_zoom_to_location());
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
        auto tab = std::make_shared<CreateLibraryTab>(mApp);
        connect(
            tab.get(), &CreateLibraryTab::libraryCreated, this,
            [this](const FilePath& fp) { openLibraryTab(fp, true); },
            Qt::QueuedConnection);
        addTab(tab);
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
    case ui::Action::LibraryPanelCheckForUpdates: {
      mApp.getRemoteLibraries().checkForUpdates();
      break;
    }
    case ui::Action::LibraryPanelCancelUpdateCheck: {
      mApp.getRemoteLibraries().cancelUpdateCheck();
      break;
    }
    case ui::Action::LibraryPanelToggleAll: {
      mApp.getRemoteLibraries().toggleAll();
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
      addSection(section + 1, true);
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

void MainWindow::triggerLibrary(slint::SharedString path,
                                ui::LibraryAction a) noexcept {
  const FilePath fp(s2q(path));
  if ((!fp.isValid()) ||
      (!fp.isLocatedInDir(mApp.getWorkspace().getLibrariesPath()))) {
    qWarning() << "Invalid path in triggerLibrary():" << s2q(path);
    return;
  }

  switch (a) {
    case ui::LibraryAction::Open: {
      openLibraryTab(fp, false);
      break;
    }
    case ui::LibraryAction::Uninstall: {
      try {
        mApp.closeLibrary(fp);
        FileUtils::removeDirRecursively(fp);  // can throw
      } catch (const Exception& e) {
        // TODO: This should be implemented without message box some day...
        QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
      }
      mApp.getWorkspace().getLibraryDb().startLibraryRescan();
      break;
    }
    case ui::LibraryAction::NewComponentCategory: {
      if (auto editor = mApp.getLibrary(fp)) {
        openComponentCategoryTab(*editor, FilePath(), false);
      }
      break;
    }
    case ui::LibraryAction::NewPackageCategory: {
      if (auto editor = mApp.getLibrary(fp)) {
        openPackageCategoryTab(*editor, FilePath(), false);
      }
      break;
    }
    case ui::LibraryAction::NewSymbol: {
      if (auto editor = mApp.getLibrary(fp)) {
        openSymbolTab(*editor, FilePath(), false);
      }
      break;
    }
    case ui::LibraryAction::NewPackage: {
      if (auto editor = mApp.getLibrary(fp)) {
        openPackageTab(*editor, FilePath(), false);
      }
      break;
    }
    case ui::LibraryAction::NewComponent: {
      if (auto editor = mApp.getLibrary(fp)) {
        openComponentTab(*editor, FilePath(), false);
      }
      break;
    }
    case ui::LibraryAction::NewDevice: {
      if (auto editor = mApp.getLibrary(fp)) {
        openDeviceTab(*editor, FilePath(), false);
      }
      break;
    }
    default: {
      qWarning() << "Unhandled action in triggerLibrary():"
                 << static_cast<int>(a);
      break;
    }
  }
}

void MainWindow::triggerLibraryElement(slint::SharedString path,
                                       ui::LibraryElementAction a) noexcept {
  const FilePath fp(s2q(path));

  switch (a) {
    case ui::LibraryElementAction::Open: {
      if (switchToLibraryElementTab<LibraryTab>(fp)) return;
      if (switchToLibraryElementTab<ComponentCategoryTab>(fp)) return;
      if (switchToLibraryElementTab<PackageCategoryTab>(fp)) return;
      if (switchToLibraryElementTab<SymbolTab>(fp)) return;
      if (switchToLibraryElementTab<PackageTab>(fp)) return;
      if (switchToLibraryElementTab<ComponentTab>(fp)) return;
      if (switchToLibraryElementTab<DeviceTab>(fp)) return;
      if (mApp.getLibrary(fp)) {
        openLibraryTab(fp, false);
      }
      break;
    }
    case ui::LibraryElementAction::Close: {
      if (auto lib = mApp.getLibrary(fp)) {
        if (lib->requestClose()) {
          mApp.closeLibrary(fp);
        }
      }
      break;
    }
    case ui::LibraryElementAction::OpenFolder: {
      DesktopServices ds(mApp.getWorkspace().getSettings());
      ds.openLocalPath(fp);
      break;
    }
    case ui::LibraryElementAction::ImportEagleLibrary: {
      if (mApp.getLibrary(fp)) {
        EagleLibraryImportWizard wiz(mApp.getWorkspace(), fp,
                                     qApp->activeWindow());
        wiz.exec();
      }
      break;
    }
    case ui::LibraryElementAction::ImportKicadLibrary: {
      if (mApp.getLibrary(fp)) {
        KiCadLibraryImportWizard wiz(mApp.getWorkspace(), fp,
                                     qApp->activeWindow());
        wiz.exec();
      }
      break;
    }
    default: {
      qWarning() << "Unhandled action in MainWindow::triggerLibraryElement():"
                 << static_cast<int>(a);
      break;
    }
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

void MainWindow::openLibraryTab(const FilePath& fp, bool wizardMode) noexcept {
  if (auto editor = mApp.openLibrary(fp)) {
    if (!switchToLibraryElementTab<LibraryTab>(fp)) {
      auto tab = std::make_shared<LibraryTab>(*editor, wizardMode);
      connect(tab.get(), &LibraryTab::componentCategoryEditorRequested, this,
              &MainWindow::openComponentCategoryTab);
      connect(tab.get(), &LibraryTab::packageCategoryEditorRequested, this,
              &MainWindow::openPackageCategoryTab);
      connect(tab.get(), &LibraryTab::symbolEditorRequested, this,
              &MainWindow::openSymbolTab);
      connect(tab.get(), &LibraryTab::packageEditorRequested, this,
              &MainWindow::openPackageTab);
      connect(tab.get(), &LibraryTab::componentEditorRequested, this,
              &MainWindow::openComponentTab);
      connect(tab.get(), &LibraryTab::deviceEditorRequested, this,
              &MainWindow::openDeviceTab);
      addTab(tab);
    }
  }
}

void MainWindow::openComponentCategoryTab(LibraryEditor& editor,
                                          const FilePath& fp,
                                          bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<ComponentCategoryTab>(fp))) {
    try {
      std::unique_ptr<ComponentCategory> cat;
      ComponentCategoryTab::Mode mode = ComponentCategoryTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        cat = ComponentCategory::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = ComponentCategoryTab::Mode::New;
        cat.reset(new ComponentCategory(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            ElementName("New Component Category"), QString(), QString()));
        if (copyFrom) {
          mode = ComponentCategoryTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<ComponentCategory> src =
              ComponentCategory::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          cat->setNames(copyLibraryElementNames(src->getNames()));
          cat->setDescriptions(src->getDescriptions());
          cat->setKeywords(src->getKeywords());
          cat->setMessageApprovals(src->getMessageApprovals());
          cat->setParentUuid(src->getParentUuid());
        }
      }
      addTab(
          std::make_shared<ComponentCategoryTab>(editor, std::move(cat), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

void MainWindow::openPackageCategoryTab(LibraryEditor& editor,
                                        const FilePath& fp,
                                        bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<PackageCategoryTab>(fp))) {
    try {
      std::unique_ptr<PackageCategory> cat;
      PackageCategoryTab::Mode mode = PackageCategoryTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        cat = PackageCategory::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = PackageCategoryTab::Mode::New;
        cat.reset(new PackageCategory(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            ElementName("New Package Category"), QString(), QString()));
        if (copyFrom) {
          mode = PackageCategoryTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<PackageCategory> src =
              PackageCategory::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          cat->setNames(copyLibraryElementNames(src->getNames()));
          cat->setDescriptions(src->getDescriptions());
          cat->setKeywords(src->getKeywords());
          cat->setMessageApprovals(src->getMessageApprovals());
          cat->setParentUuid(src->getParentUuid());
        }
      }
      addTab(
          std::make_shared<PackageCategoryTab>(editor, std::move(cat), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

void MainWindow::openSymbolTab(LibraryEditor& editor, const FilePath& fp,
                               bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<SymbolTab>(fp))) {
    try {
      std::unique_ptr<Symbol> sym;
      SymbolTab::Mode mode = SymbolTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        sym = Symbol::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = SymbolTab::Mode::New;
        sym.reset(new Symbol(Uuid::createRandom(), Version::fromString("0.1"),
                             mApp.getWorkspace().getSettings().userName.get(),
                             ElementName("New Symbol"), QString(), QString()));
        if (copyFrom) {
          mode = SymbolTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<Symbol> src =
              Symbol::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          sym->setNames(copyLibraryElementNames(src->getNames()));
          sym->setDescriptions(src->getDescriptions());
          sym->setKeywords(src->getKeywords());
          sym->setMessageApprovals(src->getMessageApprovals());
          sym->setCategories(src->getCategories());
          sym->setResources(src->getResources());
          sym->setGridInterval(src->getGridInterval());
          QSet<QString> filesToCopy;
          // Copy pins but generate new UUIDs.
          for (const SymbolPin& pin : src->getPins()) {
            sym->getPins().append(std::make_shared<SymbolPin>(
                Uuid::createRandom(), pin.getName(), pin.getPosition(),
                pin.getLength(), pin.getRotation(), pin.getNamePosition(),
                pin.getNameRotation(), pin.getNameHeight(),
                pin.getNameAlignment()));
          }
          // Copy polygons but generate new UUIDs.
          for (const Polygon& polygon : src->getPolygons()) {
            sym->getPolygons().append(std::make_shared<Polygon>(
                Uuid::createRandom(), polygon.getLayer(),
                polygon.getLineWidth(), polygon.isFilled(),
                polygon.isGrabArea(), polygon.getPath()));
          }
          // Copy circles but generate new UUIDs.
          for (const Circle& circle : src->getCircles()) {
            sym->getCircles().append(std::make_shared<Circle>(
                Uuid::createRandom(), circle.getLayer(), circle.getLineWidth(),
                circle.isFilled(), circle.isGrabArea(), circle.getCenter(),
                circle.getDiameter()));
          }
          // Copy texts but generate new UUIDs.
          for (const Text& text : src->getTexts()) {
            sym->getTexts().append(std::make_shared<Text>(
                Uuid::createRandom(), text.getLayer(), text.getText(),
                text.getPosition(), text.getRotation(), text.getHeight(),
                text.getAlign(), text.isLocked()));
          }
          // Copy images but generate new UUIDs.
          for (const Image& image : src->getImages()) {
            sym->getImages().append(
                std::make_shared<Image>(Uuid::createRandom(), image));
            filesToCopy.insert(*image.getFileName());
          }
          // Copy all referenced files.
          for (const QString& fileName : filesToCopy) {
            if (src->getDirectory().fileExists(fileName)) {
              sym->getDirectory().write(fileName,
                                        src->getDirectory().read(fileName));
            }
          }
        }
      }
      addTab(std::make_shared<SymbolTab>(editor, std::move(sym), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

void MainWindow::openPackageTab(LibraryEditor& editor, const FilePath& fp,
                                bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<PackageTab>(fp))) {
    try {
      std::unique_ptr<Package> pkg;
      PackageTab::Mode mode = PackageTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        pkg = Package::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = PackageTab::Mode::New;
        pkg.reset(new Package(Uuid::createRandom(), Version::fromString("0.1"),
                              mApp.getWorkspace().getSettings().userName.get(),
                              ElementName("New Package"), QString(), QString(),
                              Package::AssemblyType::Auto));
        if (copyFrom) {
          mode = PackageTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<Package> src =
              Package::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          pkg->setNames(copyLibraryElementNames(src->getNames()));
          pkg->setDescriptions(src->getDescriptions());
          pkg->setKeywords(src->getKeywords());
          pkg->setMessageApprovals(src->getMessageApprovals());
          pkg->setCategories(src->getCategories());
          pkg->setResources(src->getResources());
          pkg->setAssemblyType(src->getAssemblyType(false));
          pkg->setGridInterval(src->getGridInterval());
          pkg->setMinCopperClearance(src->getMinCopperClearance());
          // Copy pads but generate new UUIDs.
          QHash<Uuid, std::optional<Uuid>> padUuidMap;
          for (const PackagePad& pad : src->getPads()) {
            const Uuid newUuid = Uuid::createRandom();
            padUuidMap.insert(pad.getUuid(), newUuid);
            pkg->getPads().append(
                std::make_shared<PackagePad>(newUuid, pad.getName()));
          }
          // Copy 3D models but generate new UUIDs.
          QHash<Uuid, std::optional<Uuid>> modelsUuidMap;
          for (const PackageModel& model : src->getModels()) {
            auto newModel = std::make_shared<PackageModel>(Uuid::createRandom(),
                                                           model.getName());
            modelsUuidMap.insert(model.getUuid(), newModel->getUuid());
            pkg->getModels().append(newModel);
            const QByteArray fileContent =
                src->getDirectory().readIfExists(model.getFileName());
            if (!fileContent.isNull()) {
              pkg->getDirectory().write(newModel->getFileName(), fileContent);
            }
          }
          // Copy footprints but generate new UUIDs.
          for (const Footprint& footprint : src->getFootprints()) {
            // Don't copy translations as they would need to be adjusted anyway.
            std::shared_ptr<Footprint> newFootprint(new Footprint(
                Uuid::createRandom(), footprint.getNames().getDefaultValue(),
                footprint.getDescriptions().getDefaultValue()));
            newFootprint->setModelPosition(footprint.getModelPosition());
            newFootprint->setModelRotation(footprint.getModelRotation());
            // Copy models but with the new UUIDs.
            QSet<Uuid> models;
            foreach (const Uuid& uuid, footprint.getModels()) {
              if (auto newUuid = modelsUuidMap.value(uuid)) {
                models.insert(*newUuid);
              }
            }
            newFootprint->setModels(models);
            // Copy pads but generate new UUIDs.
            for (const FootprintPad& pad : footprint.getPads()) {
              std::optional<Uuid> pkgPad = pad.getPackagePadUuid();
              if (pkgPad) {
                pkgPad = padUuidMap.value(*pkgPad);  // Translate to new UUID
              }
              newFootprint->getPads().append(std::make_shared<FootprintPad>(
                  Uuid::createRandom(), pkgPad, pad.getPosition(),
                  pad.getRotation(), pad.getShape(), pad.getWidth(),
                  pad.getHeight(), pad.getRadius(), pad.getCustomShapeOutline(),
                  pad.getStopMaskConfig(), pad.getSolderPasteConfig(),
                  pad.getCopperClearance(), pad.getComponentSide(),
                  pad.getFunction(), pad.getHoles()));
            }
            // Copy polygons but generate new UUIDs.
            for (const Polygon& polygon : footprint.getPolygons()) {
              newFootprint->getPolygons().append(std::make_shared<Polygon>(
                  Uuid::createRandom(), polygon.getLayer(),
                  polygon.getLineWidth(), polygon.isFilled(),
                  polygon.isGrabArea(), polygon.getPath()));
            }
            // Copy circles but generate new UUIDs.
            for (const Circle& circle : footprint.getCircles()) {
              newFootprint->getCircles().append(std::make_shared<Circle>(
                  Uuid::createRandom(), circle.getLayer(),
                  circle.getLineWidth(), circle.isFilled(), circle.isGrabArea(),
                  circle.getCenter(), circle.getDiameter()));
            }
            // Copy stroke texts but generate new UUIDs.
            for (const StrokeText& text : footprint.getStrokeTexts()) {
              newFootprint->getStrokeTexts().append(
                  std::make_shared<StrokeText>(
                      Uuid::createRandom(), text.getLayer(), text.getText(),
                      text.getPosition(), text.getRotation(), text.getHeight(),
                      text.getStrokeWidth(), text.getLetterSpacing(),
                      text.getLineSpacing(), text.getAlign(),
                      text.getMirrored(), text.getAutoRotate()));
            }
            // Copy zones but generate new UUIDs.
            for (const Zone& zone : footprint.getZones()) {
              newFootprint->getZones().append(
                  std::make_shared<Zone>(Uuid::createRandom(), zone));
            }
            // Copy holes but generate new UUIDs.
            for (const Hole& hole : footprint.getHoles()) {
              newFootprint->getHoles().append(std::make_shared<Hole>(
                  Uuid::createRandom(), hole.getDiameter(), hole.getPath(),
                  hole.getStopMaskConfig()));
            }
            pkg->getFootprints().append(newFootprint);
          }
        } else {
          pkg->getFootprints().append(std::make_shared<Footprint>(
              Uuid::createRandom(), ElementName("default"), ""));
        }
      }
      addTab(std::make_shared<PackageTab>(editor, std::move(pkg), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

void MainWindow::openComponentTab(LibraryEditor& editor, const FilePath& fp,
                                  bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<ComponentTab>(fp))) {
    try {
      std::unique_ptr<Component> cmp;
      ComponentTab::Mode mode = ComponentTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        cmp = Component::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = ComponentTab::Mode::New;
        cmp.reset(
            new Component(Uuid::createRandom(), Version::fromString("0.1"),
                          mApp.getWorkspace().getSettings().userName.get(),
                          ElementName("New Component"), QString(), QString()));
        if (copyFrom) {
          mode = ComponentTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<Component> src =
              Component::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          cmp->setNames(copyLibraryElementNames(src->getNames()));
          cmp->setDescriptions(src->getDescriptions());
          cmp->setKeywords(src->getKeywords());
          cmp->setMessageApprovals(src->getMessageApprovals());
          cmp->setCategories(src->getCategories());
          cmp->setResources(src->getResources());
          cmp->setIsSchematicOnly(src->isSchematicOnly());
          cmp->getAttributes() = src->getAttributes();
          cmp->setDefaultValue(src->getDefaultValue());
          cmp->setPrefixes(src->getPrefixes());
          // Copy signals but generate new UUIDs.
          QHash<Uuid, Uuid> signalUuidMap;
          for (const ComponentSignal& signal : src->getSignals()) {
            const Uuid newUuid = Uuid::createRandom();
            signalUuidMap.insert(signal.getUuid(), newUuid);
            cmp->getSignals().append(std::make_shared<ComponentSignal>(
                newUuid, signal.getName(), signal.getRole(),
                signal.getForcedNetName(), signal.isRequired(),
                signal.isNegated(), signal.isClock()));
          }
          // Copy symbol variants but generate new UUIDs.
          for (const ComponentSymbolVariant& var : src->getSymbolVariants()) {
            // don't copy translations as they would need to be adjusted anyway
            std::shared_ptr<ComponentSymbolVariant> copy(
                new ComponentSymbolVariant(
                    Uuid::createRandom(), var.getNorm(),
                    var.getNames().getDefaultValue(),
                    var.getDescriptions().getDefaultValue()));
            // Copy gates.
            for (const ComponentSymbolVariantItem& item :
                 var.getSymbolItems()) {
              std::shared_ptr<ComponentSymbolVariantItem> gateCopy(
                  new ComponentSymbolVariantItem(
                      Uuid::createRandom(), item.getSymbolUuid(),
                      item.getSymbolPosition(), item.getSymbolRotation(),
                      item.isRequired(), item.getSuffix()));
              // Copy pin-signal-map.
              for (const ComponentPinSignalMapItem& map :
                   item.getPinSignalMap()) {
                std::optional<Uuid> signal = map.getSignalUuid();
                if (signal) {
                  signal = *signalUuidMap.find(*map.getSignalUuid());
                }
                gateCopy->getPinSignalMap().append(
                    std::make_shared<ComponentPinSignalMapItem>(
                        map.getPinUuid(), signal, map.getDisplayType()));
              }
              copy->getSymbolItems().append(gateCopy);
            }
            cmp->getSymbolVariants().append(copy);
          }
        } else {
          cmp->getSymbolVariants().append(
              std::make_shared<ComponentSymbolVariant>(
                  Uuid::createRandom(), "", ElementName("default"), ""));
        }
      }
      addTab(std::make_shared<ComponentTab>(editor, std::move(cmp), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

void MainWindow::openDeviceTab(LibraryEditor& editor, const FilePath& fp,
                               bool copyFrom) noexcept {
  if (copyFrom || (!switchToLibraryElementTab<DeviceTab>(fp))) {
    try {
      std::unique_ptr<Device> dev;
      DeviceTab::Mode mode = DeviceTab::Mode::Open;
      if (fp.isValid() && (!copyFrom)) {
        auto fs = TransactionalFileSystem::open(
            fp, editor.isWritable(), &askForRestoringBackup,
            DirectoryLockHandlerDialog::createDirectoryLockCallback());
        dev = Device::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));
      } else {
        mode = DeviceTab::Mode::New;
        dev.reset(new Device(Uuid::createRandom(), Version::fromString("0.1"),
                             mApp.getWorkspace().getSettings().userName.get(),
                             ElementName("New Device"), QString(), QString(),
                             Uuid::createRandom(), Uuid::createRandom()));
        if (copyFrom) {
          mode = DeviceTab::Mode::Duplicate;
          auto fs = TransactionalFileSystem::openRO(fp, &askForRestoringBackup);
          std::unique_ptr<Device> src =
              Device::open(std::unique_ptr<TransactionalDirectory>(
                  new TransactionalDirectory(fs)));
          dev->setNames(copyLibraryElementNames(src->getNames()));
          dev->setDescriptions(src->getDescriptions());
          dev->setKeywords(src->getKeywords());
          dev->setMessageApprovals(src->getMessageApprovals());
          dev->setCategories(src->getCategories());
          dev->setResources(src->getResources());
          dev->setComponentUuid(src->getComponentUuid());
          dev->setPackageUuid(src->getPackageUuid());
          dev->getPadSignalMap() = src->getPadSignalMap();
          dev->getAttributes() = src->getAttributes();
          dev->getParts() = src->getParts();
        }
      }
      addTab(std::make_shared<DeviceTab>(editor, std::move(dev), mode));
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

std::shared_ptr<SchematicTab> MainWindow::openSchematicTab(int projectIndex,
                                                           int index) noexcept {
  if (auto tab = switchToProjectTab<SchematicTab>(projectIndex, index)) {
    return tab;
  } else if (auto prjEditor = mApp.getProjects().value(projectIndex)) {
    if (auto schEditor = prjEditor->getSchematics().value(index)) {
      auto tab = std::make_shared<SchematicTab>(mApp, *schEditor);
      addTab(tab);
      return tab;
    }
  }
  return nullptr;
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

void MainWindow::updateHomeTabSection() noexcept {
  for (int i = 0; i < mSections->count(); ++i) {
    mSections->at(i)->setHomeTabVisible(i == 0);
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
bool MainWindow::switchToLibraryElementTab(const FilePath& fp) noexcept {
  for (auto section : *mSections) {
    if (section->switchToLibraryElementTab<T>(fp)) {
      return true;
    }
  }
  return false;
}

template <typename T>
std::shared_ptr<T> MainWindow::switchToProjectTab(int prjIndex,
                                                  int objIndex) noexcept {
  for (auto section : *mSections) {
    if (auto tab = section->switchToProjectTab<T>(prjIndex, objIndex)) {
      return tab;
    }
  }
  return nullptr;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
