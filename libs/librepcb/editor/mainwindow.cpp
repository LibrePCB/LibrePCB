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
#include "library/org/organizationtab.h"
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
#include "utils/editortoolbox.h"
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
#include <librepcb/core/library/org/organization.h>
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
  static const QString value =
      qgetenv("LIBREPCB_WINDOW_SIZE").trimmed().toLower();
  if (value.isEmpty()) {
    return std::nullopt;  // Environment variable not set.
  }
  const QStringList numbers = value.split("x");
  const int width = numbers.value(0).toInt();
  const int height = numbers.value(1).toInt();
  return QSize(width, height);
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

template <typename T>
static void duplicateLibraryElement(T& element, const T& other) {
  element.duplicateFrom(other);
  element.setCreated(QDateTime::currentDateTime());
  element.setNames(copyLibraryElementNames(other.getNames()));
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int id,
                       QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mId(id),
    mSettingsPrefix(QString("window_%1").arg(mId)),
    mApp(app),
    mWindow(win),
    mWidget(static_cast<QWidget*>(slint::cbindgen_private::slint_qt_get_widget(
        &mWindow->window().window_handle()))),
    mSections(new UiObjectList<WindowSection, ui::WindowSectionData>()),
    mProjectPreviewRenderer(new ProjectReadmeRenderer(this)),
    mTestAdapter() {
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
      const qreal scale = mWidget->devicePixelRatioF();
      return section->renderScene(width * scale, height * scale, scene);
    } else {
      return slint::Image();
    }
  });
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  slint::private_api::PointerEvent e,
                                  int scene) {
    if (auto section = mSections->value(sectionIndex)) {
      const qreal scale = mWidget->devicePixelRatioF();
      section->processScenePointerEvent(QPointF(x * scale, y * scale), e,
                                        scene);
    }
  });
  b.on_scene_scrolled([this](int sectionIndex, float x, float y,
                             slint::private_api::PointerScrollEvent e,
                             int scene) {
    if (auto section = mSections->value(sectionIndex)) {
      const qreal scale = mWidget->devicePixelRatioF();
      return section->processSceneScrolled(QPointF(x * scale, y * scale), e,
                                           scene);
    }
    return false;
  });
  b.on_scene_key_pressed(
      [this](int sectionIndex, const slint::language::KeyEvent& e) {
        if (auto section = mSections->value(sectionIndex)) {
          return section->processSceneKeyPressed(e);
        }
        return false;
      });
  b.on_scene_key_released(
      [this](int sectionIndex, const slint::language::KeyEvent& e) {
        if (auto section = mSections->value(sectionIndex)) {
          return section->processSceneKeyReleased(e);
        }
        return false;
      });
  b.on_request_project_preview(
      [this](const slint::SharedString& fp, float width, const ui::Theme&) {
        // Theme is not used, but needs to be a dependency of this callback
        // to trigger re-rendering when the theme colors have changed.
        const qreal scale = mWidget->devicePixelRatioF();
        mProjectPreviewRenderer->request(FilePath(s2q(fp)),
                                         static_cast<int>(width * scale));
        return true;
      });

  // Update UI state.
  d.fn_current_tab_changed();

  // Apply UI theme.
  auto applyTheme = [this]() {
    mWindow->global<ui::Data>().set_theme(l2s(mApp.getTheme()));
    mWindow->window().request_redraw();  // Workaround for spurious wrong colors
  };
  applyTheme();
  connect(&mApp.getWorkspace().getSettings().uiTheme,
          &WorkspaceSettingsItem::edited, this, applyTheme,
          Qt::QueuedConnection);

  // Update editor command translations & keyboard shortcuts.
  EditorCommandSetUpdater::update(mWindow->global<ui::EditorCommandSet>());
  connect(&mApp.getWorkspace().getSettings().keyboardShortcuts,
          &WorkspaceSettingsItem_KeyboardShortcuts::edited, this, [this]() {
            EditorCommandSetUpdater::update(
                mWindow->global<ui::EditorCommandSet>());
          });

  // Setup test adapter.
  if (qgetenv("LIBREPCB_ENABLE_TEST_ADAPTER") == "1") {
    mTestAdapter.reset(new MainWindowTestAdapter(app, *this, mWidget));
    connect(
        mTestAdapter.get(), &MainWindowTestAdapter::actionTriggered, this,
        [this](ui::Action a) { trigger(a); }, Qt::QueuedConnection);
  }

  // Show window.
  mWindow->show();

  // Load window state.
  QSettings cs;
  const QByteArray geometry =
      cs.value(mSettingsPrefix % "/geometry").toByteArray();
  if (auto size = getOverrideWindowSize()) {
    if (!size->isEmpty()) {
      qInfo() << "Window size enforced to" << *size;
      mWidget->resize(*size);
    } else {
      qInfo() << "Window resizing explicitly disabled by environment variable.";
    }
  } else if (geometry.isEmpty()) {
    // By default, open the window maximized as this is more intuitive than a
    // small window with hardcoded, screen-independent size in the Slint file
    // (https://github.com/LibrePCB/LibrePCB/issues/355). However, do this only
    // the very first time (if no geometry is stored yet) to avoid interfering
    // with tiling window managers.
    qInfo() << "No window geometry to restore, thus maximizing.";
    mWidget->setWindowState(Qt::WindowMaximized | Qt::WindowActive);
  } else if (!mWidget->restoreGeometry(geometry)) {
    // This can happen if a tiling window manager is used, which is fine
    // since the user wants to control window geometry then.
    qInfo() << "Failed to restore window geometry, e.g. due to window manager.";
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
  std::shared_ptr<WindowSection> s =
      std::make_shared<WindowSection>(mApp, *this);
  connect(s.get(), &WindowSection::currentTabChanged, this, [this]() {
    const ui::Data& d = mWindow->global<ui::Data>();
    d.fn_current_tab_changed();
  });
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

void MainWindow::openLibraryTab(const FilePath& fp, bool wizardMode) noexcept {
  if (auto editor = mApp.openLibrary(fp)) {
    if (!switchToLibraryElementTab<LibraryTab>(fp)) {
      addTab(std::make_shared<LibraryTab>(*editor, wizardMode));
    }
  }
}

void MainWindow::openComponentCategoryTab(LibraryEditor& editor,
                                          const FilePath& fp) noexcept {
  openLibraryElementTab<ComponentCategory, ComponentCategoryTab>(editor, fp);
}

bool MainWindow::openNewComponentCategoryTab(
    LibraryEditor& editor, const FilePath& duplicateFromFp,
    const ComponentCategory* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<ComponentCategory, ComponentCategoryTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        return std::unique_ptr<ComponentCategory>(new ComponentCategory(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            QDateTime::currentDateTime(), ElementName("New Component Category"),
            QString(), QString()));
      });
}

void MainWindow::openPackageCategoryTab(LibraryEditor& editor,
                                        const FilePath& fp) noexcept {
  openLibraryElementTab<PackageCategory, PackageCategoryTab>(editor, fp);
}

bool MainWindow::openNewPackageCategoryTab(
    LibraryEditor& editor, const FilePath& duplicateFromFp,
    const PackageCategory* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<PackageCategory, PackageCategoryTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        return std::unique_ptr<PackageCategory>(new PackageCategory(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            QDateTime::currentDateTime(), ElementName("New Package Category"),
            QString(), QString()));
      });
}

void MainWindow::openSymbolTab(LibraryEditor& editor,
                               const FilePath& fp) noexcept {
  openLibraryElementTab<Symbol, SymbolTab>(editor, fp);
}

bool MainWindow::openNewSymbolTab(LibraryEditor& editor,
                                  const FilePath& duplicateFromFp,
                                  const Symbol* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<Symbol, SymbolTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        return std::unique_ptr<Symbol>(
            new Symbol(Uuid::createRandom(), Version::fromString("0.1"),
                       mApp.getWorkspace().getSettings().userName.get(),
                       QDateTime::currentDateTime(), ElementName("New Symbol"),
                       QString(), QString()));
      });
}

void MainWindow::openPackageTab(LibraryEditor& editor,
                                const FilePath& fp) noexcept {
  openLibraryElementTab<Package, PackageTab>(editor, fp);
}

bool MainWindow::openNewPackageTab(LibraryEditor& editor,
                                   const FilePath& duplicateFromFp,
                                   const Package* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<Package, PackageTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        std::unique_ptr<Package> obj(new Package(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            QDateTime::currentDateTime(), ElementName("New Package"), QString(),
            QString(), Package::AssemblyType::Auto));
        obj->getFootprints().append(std::make_shared<Footprint>(
            Uuid::createRandom(), ElementName("default"), ""));
        return obj;
      });
}

void MainWindow::openComponentTab(LibraryEditor& editor,
                                  const FilePath& fp) noexcept {
  openLibraryElementTab<Component, ComponentTab>(editor, fp);
}

bool MainWindow::openNewComponentTab(
    LibraryEditor& editor, const FilePath& duplicateFromFp,
    const Component* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<Component, ComponentTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        std::unique_ptr<Component> obj(
            new Component(Uuid::createRandom(), Version::fromString("0.1"),
                          mApp.getWorkspace().getSettings().userName.get(),
                          QDateTime::currentDateTime(),
                          ElementName("New Component"), QString(), QString()));
        obj->getSymbolVariants().append(
            std::make_shared<ComponentSymbolVariant>(
                Uuid::createRandom(), "", ElementName("default"), ""));
        return obj;
      });
}

void MainWindow::openDeviceTab(LibraryEditor& editor,
                               const FilePath& fp) noexcept {
  openLibraryElementTab<Device, DeviceTab>(editor, fp);
}

bool MainWindow::openNewDeviceTab(LibraryEditor& editor,
                                  const FilePath& duplicateFromFp,
                                  const Device* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<Device, DeviceTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        return std::unique_ptr<Device>(new Device(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            QDateTime::currentDateTime(), ElementName("New Device"), QString(),
            QString(), Uuid::createRandom(), Uuid::createRandom()));
      });
}

void MainWindow::openOrganizationTab(LibraryEditor& editor,
                                     const FilePath& fp) noexcept {
  openLibraryElementTab<Organization, OrganizationTab>(editor, fp);
}

bool MainWindow::openNewOrganizationTab(
    LibraryEditor& editor, const FilePath& duplicateFromFp,
    const Organization* duplicateFromObj) noexcept {
  return openNewLibraryElementTab<Organization, OrganizationTab>(
      editor, duplicateFromFp, duplicateFromObj, [this]() {
        return std::unique_ptr<Organization>(new Organization(
            Uuid::createRandom(), Version::fromString("0.1"),
            mApp.getWorkspace().getSettings().userName.get(),
            QDateTime::currentDateTime(), ElementName("New Organization"),
            QString(), QString()));
      });
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

void MainWindow::openBoard2dTab(int projectIndex, int index,
                                bool switchToTab) noexcept {
  if (!switchToProjectTab<Board2dTab>(projectIndex, index)) {
    if (auto prjEditor = mApp.getProjects().value(projectIndex)) {
      if (auto brdEditor = prjEditor->getBoards().value(index)) {
        addTab(std::make_shared<Board2dTab>(mApp, *brdEditor), -1, -1,
               switchToTab, switchToTab);
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

void MainWindow::requestComponentTab(const FilePath& fp) noexcept {
  if (auto editor = mApp.openLibrary(fp.getParentDir().getParentDir())) {
    openComponentTab(*editor, fp);
  }
}

void MainWindow::requestPackageTab(const FilePath& fp) noexcept {
  if (auto editor = mApp.openLibrary(fp.getParentDir().getParentDir())) {
    openPackageTab(*editor, fp);
  }
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
        // Abort any currently running library rescan since this can cause
        // problems due to concurrent file access.
        mApp.getWorkspace().getLibraryDb().cancelLibraryRescan();
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
        openNewComponentCategoryTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewPackageCategory: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewPackageCategoryTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewSymbol: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewSymbolTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewPackage: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewPackageTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewComponent: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewComponentTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewDevice: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewDeviceTab(*editor);
      }
      break;
    }
    case ui::LibraryAction::NewOrganization: {
      if (auto editor = mApp.getLibrary(fp)) {
        openNewOrganizationTab(*editor);
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
      if (switchToLibraryElementTab<OrganizationTab>(fp)) return;
      if (mApp.getLibrary(fp)) {
        openLibraryTab(fp, false);
      }
      break;
    }
    case ui::LibraryElementAction::Close: {
      if (auto lib = mApp.getLibrary(fp)) {
        if (lib->requestCloseAllTabs()) {
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
        EagleLibraryImportWizard wiz(mApp.getTheme(), mApp.getWorkspace(), fp,
                                     mWidget);
        wiz.exec();
      }
      break;
    }
    case ui::LibraryElementAction::ImportKicadLibrary: {
      if (mApp.getLibrary(fp)) {
        KiCadLibraryImportWizard wiz(mApp.getTheme(), mApp.getWorkspace(), fp,
                                     mWidget);
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

void MainWindow::updateHomeTabSection() noexcept {
  for (int i = 0; i < mSections->count(); ++i) {
    mSections->at(i)->setHomeTabVisible(i == 0);
  }
}

template <typename Element, typename Tab>
void MainWindow::openLibraryElementTab(LibraryEditor& editor,
                                       const FilePath& fp) noexcept {
  if (!switchToLibraryElementTab<Tab>(fp)) {
    try {
      auto fs = TransactionalFileSystem::open(
          fp, editor.isWritable(), &askForRestoringBackup,
          DirectoryLockHandlerDialog::createDirectoryLockCallback());
      std::unique_ptr<Element> obj =
          Element::open(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(fs)));
      addTab(std::make_shared<Tab>(editor, std::move(obj), Tab::Mode::Open));
    } catch (const UserCanceled& e) {
    } catch (const Exception& e) {
      QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
    }
  }
}

template <typename Element, typename Tab>
bool MainWindow::openNewLibraryElementTab(
    LibraryEditor& editor, const FilePath& duplicateFromFp,
    const Element* duplicateFromObj,
    std::function<std::unique_ptr<Element>()> factory) noexcept {
  try {
    auto obj = factory();
    std::unique_ptr<Element> src;
    if ((!duplicateFromObj) && duplicateFromFp.isValid()) {
      auto fs = TransactionalFileSystem::openRO(duplicateFromFp,
                                                &askForRestoringBackup);
      src = Element::open(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(fs)));
      duplicateFromObj = src.get();
    }
    if (duplicateFromObj) {
      duplicateLibraryElement(*obj, *duplicateFromObj);
    }
    addTab(std::make_shared<Tab>(
        editor, std::move(obj),
        duplicateFromObj ? Tab::Mode::Duplicate : Tab::Mode::New));
    return true;
  } catch (const UserCanceled& e) {
  } catch (const Exception& e) {
    QMessageBox::critical(mWidget, tr("Error"), e.getMsg());
  }
  return false;
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
