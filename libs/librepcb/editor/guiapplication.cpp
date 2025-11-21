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
#include "graphics/graphicslayerlist.h"
#include "library/librariesmodel.h"
#include "library/libraryeditor.h"
#include "library/libraryelementcache.h"
#include "mainwindow.h"
#include "notification.h"
#include "notificationsmodel.h"
#include "project/newprojectwizard/newprojectwizard.h"
#include "project/projecteditor.h"
#include "utils/slinthelpers.h"
#include "utils/slintkeyeventtextbuilder.h"
#include "utils/uihelpers.h"
#include "workspace/desktopintegration.h"
#include "workspace/desktopservices.h"
#include "workspace/initializeworkspacewizard/initializeworkspacewizard.h"
#include "workspace/projectlibraryupdater/projectlibraryupdater.h"
#include "workspace/quickaccessmodel.h"
#include "workspace/workspacesettingsdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/norms.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/utils/mathparser.h>
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

// Translation callback for Slint. Needs to convert gettext placeholders
// to Qt placeholders ("{1}" -> "%1"). Not very elegant for now, could
// probably be improved a lot...
void slintTr(slint::private_api::Slice<uint8_t> string,
             slint::private_api::Slice<uint8_t> ctx,
             slint::private_api::Slice<uint8_t> domain, int32_t n,
             slint::private_api::Slice<uint8_t> plural,
             slint::SharedString* out) noexcept {
  Q_UNUSED(domain);
  const QByteArray context =
      "ui::" + QByteArray(reinterpret_cast<const char*>(ctx.ptr), ctx.len);
  QByteArray str =
      (plural.len
           ? QByteArray(reinterpret_cast<const char*>(plural.ptr), plural.len)
           : QByteArray(reinterpret_cast<const char*>(string.ptr), string.len));

  // Helpers to build pattern strings "{n}" and "%n".
  QByteArray pattern;
  auto buildSlintPattern = [&pattern](int i) -> const QByteArray& {
    pattern = "{" + QByteArray::number(i) + "}";
    return pattern;
  };
  auto buildQtPattern = [&pattern](int i) -> const QByteArray& {
    pattern = "%" + QByteArray::number(i);
    return pattern;
  };

  str.replace("{n}", "%n");
  for (int i = 0; str.contains(buildSlintPattern(i)); ++i) {
    str.replace(pattern, "%" + QByteArray::number(i + 1));
  }
  for (int i = 1; str.contains("{}"); ++i) {
    str.replace(str.indexOf("{}"), 2, "%" + QByteArray::number(i));
  }
  str = QCoreApplication::translate(context.data(), str.data(), nullptr,
                                    plural.len ? n : -1)
            .toUtf8();
  str.replace("%n", "{n}");
  for (int i = 1; str.contains(buildQtPattern(i)); ++i) {
    str.replace(pattern, "{" + QByteArray::number(i - 1) + "}");
  }
  *out = std::string_view(str.data(), str.size());
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GuiApplication::GuiApplication(Workspace& ws, bool fileFormatIsOutdated,
                               QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLibrariesContainStandardComponents(false),
    mPreviewLayers(GraphicsLayerList::previewLayers(&ws.getSettings())),
    mLibraryElementCache(new LibraryElementCache(ws.getLibraryDb())),
    mNotifications(new NotificationsModel(ws)),
    mQuickAccessModel(new QuickAccessModel(ws)),
    mLocalLibraries(new LibrariesModel(ws, LibrariesModel::Mode::LocalLibs)),
    mRemoteLibraries(new LibrariesModel(ws, LibrariesModel::Mode::RemoteLibs)),
    mLibrariesFilter(new SlintKeyEventTextBuilder()),
    mProjects(new UiObjectList<ProjectEditor, ui::ProjectData>()),
    mLibraries(new UiObjectList<LibraryEditor, ui::LibraryData>()) {
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

  // It seems registering the callback *before* the first Slint window is
  // created, doesn't work for some reason so we do it here. Maybe the reason
  // is that that's not an official Slint feature but a hack from myself ;-)
  slint::private_api::slint_translate_set_translate_callback(&slintTr);

  // Setup quick access.
  connect(mQuickAccessModel.get(), &QuickAccessModel::openFileTriggered, this,
          [this](const FilePath& fp) { openFile(fp, qApp->activeWindow()); });

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

  // Show warning if the runtime resources were not found. Intended to catch
  // deployment errors and to avoid bug reports if users didn't install the
  // "share" directory.
  bool runtimeResourcesValid = false;
  const FilePath resourcesDir =
      Application::getResourcesDir(&runtimeResourcesValid);
  if (!runtimeResourcesValid) {
    mNotifications->push(std::make_shared<Notification>(
        ui::NotificationType::Critical, "Broken Installation Detected",
        QString("The runtime resources from the 'share' folder were not found "
                "at '%1', therefore the application will not work correctly. "
                "Please make sure to install all files of LibrePCB as "
                "explained in the installation instructions.")
            .arg(resourcesDir.toNative()),
        QString(), QString(), true));
  }

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
          this, [this]() {
            if (auto win = getCurrentWindow()) {
              win->showPanelPage(ui::PanelPage::Libraries);
            }
          });
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
            tr("The internal libraries database is being updated. This may "
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

  // Setup library models & filter.
  connect(mRemoteLibraries.get(), &LibrariesModel::onlineVersionsAvailable,
          mLocalLibraries.get(), &LibrariesModel::setOnlineVersions);
  connect(mRemoteLibraries.get(), &LibrariesModel::aboutToUninstallLibrary,
          this, &GuiApplication::closeLibrary);

  // Check if standard components are installed.
  connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanFinished, this,
          &GuiApplication::updateLibrariesContainStandardComponents);
  updateLibrariesContainStandardComponents();

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
}

/*******************************************************************************
 *  Workspace
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

/*******************************************************************************
 *  Libraries
 ******************************************************************************/

std::shared_ptr<LibraryEditor> GuiApplication::getLibrary(
    const FilePath& libDir) noexcept {
  for (int i = 0; i < mLibraries->count(); ++i) {
    if (mLibraries->at(i)->getFilePath() == libDir) {
      return mLibraries->at(i);
    }
  }
  return nullptr;
}

std::shared_ptr<LibraryEditor> GuiApplication::openLibrary(
    const FilePath& libDir) noexcept {
  auto switchToLibrary = [this](int index) {
    for (auto win : mWindows) {
      win->setCurrentLibrary(index);
      win->showPanelPage(ui::PanelPage::Documents);
    }
  };

  if (auto lib = getLibrary(libDir)) {
    if (auto index = mLibraries->indexOf(lib.get())) {
      switchToLibrary(*index);
    }
    return lib;
  }

  auto askForRestoringBackup = [](const FilePath&) {
    QMessageBox::StandardButton btn = QMessageBox::question(
        qApp->activeWindow(), tr("Restore autosave backup?"),
        tr("It seems that the application crashed the last time you opened "
           "this library. Do you want to restore the last autosave backup?"),
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
  };

  try {
    // Open file system.
    const bool readOnly =
        libDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());
    auto fs = TransactionalFileSystem::open(
        libDir, !readOnly, askForRestoringBackup,
        DirectoryLockHandlerDialog::createDirectoryLockCallback());  // can
                                                                     // throw

    // Open library.
    auto lib = Library::open(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(fs)));  // can throw

    // Keep handle.
    const int index = mLibraries->count();
    auto editor = std::make_shared<LibraryEditor>(*this, std::move(lib), index);
    mLibraries->insert(index, editor);
    switchToLibrary(index);
    return editor;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Failed to open library"),
                          e.getMsg());
  }

  return nullptr;
}

void GuiApplication::closeLibrary(const FilePath& libDir) noexcept {
  if (auto index = mLibraries->indexOf(getLibrary(libDir).get())) {
    mLibraries->remove(*index);
    for (int i = *index; i < mLibraries->count(); ++i) {
      mLibraries->at(i)->setUiIndex(i);
    }
  }
}

bool GuiApplication::requestClosingAllLibraries() noexcept {
  for (std::size_t i = 0; i < mLibraries->row_count(); ++i) {
    if (auto lib = mLibraries->value(i)) {
      if (!lib->requestClose()) {
        return false;
      }
    }
  }
  return true;
}

/*******************************************************************************
 *  Projects
 ******************************************************************************/

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

std::shared_ptr<ProjectEditor> GuiApplication::openProject(
    FilePath fp, QWidget* parent) noexcept {
  if (!fp.isValid()) {
    QSettings cs;  // client settings
    QString lastOpenedFile =
        cs.value("controlpanel/last_open_project", mWorkspace.getPath().toStr())
            .toString();

    fp = FilePath(FileDialog::getOpenFileName(
        parent, tr("Open Project"), lastOpenedFile,
        tr("LibrePCB project files (%1)").arg("*.lpp *.lppz")));
    if (!fp.isValid()) return nullptr;

    cs.setValue("controlpanel/last_open_project", fp.toNative());
  }

  auto switchToProject = [this](int index) {
    for (auto win : mWindows) {
      win->setCurrentProject(index);
      win->showPanelPage(ui::PanelPage::Documents);
    }
  };

  // If the same project is already open, just return it.
  const FilePath uniqueFp = fp.toUnique();
  for (int i = 0; i < mProjects->count(); ++i) {
    auto prj = mProjects->at(i);
    if (prj->getProject().getFilepath().toUnique() == uniqueFp) {
      switchToProject(i);
      return prj;
    }
  }

  // Opening the project can take some time, use wait cursor to provide
  // immediate UI feedback.
  QApplication::setOverrideCursor(Qt::WaitCursor);
  auto cursorScopeGuard =
      scopeGuard([]() { QApplication::restoreOverrideCursor(); });

  // Callback to ask for restoring backup.
  auto askForRestoringBackup = [parent](const FilePath&) {
    QMessageBox::StandardButton btn = QMessageBox::question(
        parent, tr("Restore autosave backup?"),
        tr("It seems that the application crashed the last time you opened "
           "this project. Do you want to restore the last autosave backup?"),
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
  };

  try {
    // Open file system.
    std::shared_ptr<TransactionalFileSystem> fs;
    QString projectFileName = fp.getFilename();
    if (fp.getSuffix() == "lppz") {
      fs = TransactionalFileSystem::openRO(
          FilePath::getRandomTempPath(),
          &TransactionalFileSystem::RestoreMode::no);
      fs->removeDirRecursively();  // 1) Get a clean initial state.
      fs->loadFromZip(fp);  // 2) Load files from ZIP.
      foreach (const QString& fn, fs->getFiles()) {
        if (fn.endsWith(".lpp")) {
          projectFileName = fn;
        }
      }
    } else {
      fs = TransactionalFileSystem::openRW(
          fp.getParentDir(), askForRestoringBackup,
          DirectoryLockHandlerDialog::createDirectoryLockCallback());
    }

    // Open project.
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
        projectFileName);  // can throw

    // Open editor & keep handle.
    const int index = mProjects->count();
    auto editor = std::make_shared<ProjectEditor>(
        *this, std::move(project), index, loader.getMigrationLog());
    connect(editor.get(), &ProjectEditor::statusBarMessageChanged, this,
            &GuiApplication::statusBarMessageChanged);
    connect(editor.get(), &ProjectEditor::ercMessageHighlightRequested, this,
            &GuiApplication::highlightErcMessage);
    connect(editor.get(), &ProjectEditor::projectLibraryUpdaterRequested, this,
            &GuiApplication::openProjectLibraryUpdater);
    mProjects->append(editor);

    // Switch to documents tab.
    switchToProject(index);

    // Delay updating the last opened project to avoid an issue when
    // double-clicking: https://github.com/LibrePCB/LibrePCB/issues/293
    QTimer::singleShot(
        500, this, [this, fp]() { mQuickAccessModel->pushRecentProject(fp); });

    return editor;
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
    return nullptr;
  }
}

void GuiApplication::closeProject(int index) noexcept {
  mProjects->remove(index);
  for (int i = index; i < mProjects->count(); ++i) {
    mProjects->at(i)->setUiIndex(i);
  }
}

bool GuiApplication::requestClosingAllProjects() noexcept {
  for (std::size_t i = 0; i < mProjects->row_count(); ++i) {
    if (auto prj = mProjects->value(i)) {
      if (!prj->requestClose()) {
        return false;
      }
    }
  }
  return true;
}

/*******************************************************************************
 *  Window Management
 ******************************************************************************/

void GuiApplication::createNewWindow(int id, int projectIndex) noexcept {
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

  // Create Slint window.
  auto win = ui::AppWindow::create();

  // Helper to create filtered, sorted library models.
  auto filteredLibs = [this](const std::shared_ptr<LibrariesModel>& model) {
    auto filterModel =
        std::make_shared<slint::FilterModel<ui::LibraryInfoData>>(
            model, [this](const ui::LibraryInfoData& lib) {
              auto s = mLibrariesFilter->getText().trimmed().toLower();
              return s.isEmpty() || s2q(lib.name).toLower().contains(s);
            });
    connect(mLibrariesFilter.get(), &SlintKeyEventTextBuilder::textChanged,
            this, [filterModel](const QString&) { filterModel->reset(); });
    return filterModel;
  };
  auto sortedLibs =
      [](const std::shared_ptr<slint::Model<ui::LibraryInfoData>>& model) {
        return std::make_shared<slint::SortModel<ui::LibraryInfoData>>(
            model,
            [](const ui::LibraryInfoData& a, const ui::LibraryInfoData& b) {
              if ((a.progress > 0) != (b.progress > 0)) {
                return a.progress > 0;
              } else if (a.outdated != b.outdated) {
                return a.outdated;
              } else if (a.installed_version.empty() !=
                         b.installed_version.empty()) {
                return b.installed_version.empty();
              } else if (a.recommended != b.recommended) {
                return a.recommended;
              } else {
                return a.name < b.name;
              }
            });
      };

  // Set global data.
  const ui::Data& d = win->global<ui::Data>();
  d.set_preview_mode(false);
  d.set_window_id(id);
  d.set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  d.set_about_librepcb_details(q2s(Application::buildFullVersionDetails()));
  d.set_workspace_path(mWorkspace.getPath().toNative().toUtf8().data());
  d.set_notifications(mNotifications);
  d.set_quick_access_items(mQuickAccessModel);
  d.set_local_libraries(filteredLibs(mLocalLibraries));
  d.set_remote_libraries(sortedLibs(filteredLibs(mRemoteLibraries)));
  d.set_projects(mProjects);
  d.fn_set_current_project(projectIndex);
  d.set_libraries(mLibraries);
  d.set_min_length(l2s(Length::min()));
  d.set_norms(q2s(getAvailableNorms()));

  // Populate attribute types & units.
  auto attributeTypes =
      std::make_shared<slint::VectorModel<slint::SharedString>>();
  auto attributeUnits = std::make_shared<
      slint::VectorModel<std::shared_ptr<slint::Model<slint::SharedString>>>>();
  for (auto t : AttributeType::getAllTypes()) {
    attributeTypes->push_back(q2s(t->getNameTr()));
    auto units = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (auto u : t->getAvailableUnits()) {
      units->push_back(q2s(u->getSymbolTr()));
    }
    attributeUnits->push_back(units);
  }
  d.set_attribute_types(attributeTypes);
  d.set_attribute_units(attributeUnits);

  // Register global callbacks.
  const ui::Backend& b = win->global<ui::Backend>();
  b.on_drop_tab([this](const slint::SharedString& srcData,
                       const slint::SharedString& dstData,
                       bool forceSwitchToTab) {
    const QStringList src = s2q(srcData).split(",");
    const QStringList dst = s2q(dstData).split(",");
    moveTab(src[0].toInt(), src[1].toInt(), src[2].toInt(),  //
            dst[0].toInt(), dst[1].toInt(), dst[2].toInt(), forceSwitchToTab);
  });
  b.on_open_url([this](const slint::SharedString& url) {
    DesktopServices ds(mWorkspace.getSettings());
    return ds.openUrl(QUrl(s2q(url)));
  });
  b.on_libraries_key_event(std::bind(&SlintKeyEventTextBuilder::process,
                                     mLibrariesFilter.get(),
                                     std::placeholders::_1));
  b.on_copy_to_clipboard([](const slint::SharedString& s) {
    QApplication::clipboard()->setText(s2q(s));
    return true;
  });
  b.on_format_length([](const ui::Int64& value, ui::LengthUnit unit) {
    const LengthUnit lpUnit = s2l(unit);
    return q2s(Toolbox::floatToString(lpUnit.convertToUnit(s2l(value)),
                                      lpUnit.getReasonableNumberOfDecimals(),
                                      QLocale()));
  });
  b.on_parse_length_input(
      [](slint::SharedString text, ui::LengthUnit unit, ui::Int64 minimum) {
        ui::LengthEditParseResult res{false, ui::Int64{0, 0}, unit};
        try {
          QString value = s2q(text);

          // Extract unit from string.
          if (auto parsedUnit = LengthUnit::extractFromExpression(value)) {
            res.evaluated_unit = l2s(*parsedUnit);
          }
          const LengthUnit lpUnit = s2l(res.evaluated_unit);

          // Parse expression and convert to Length.
          const MathParser::Result result = MathParser().parse(value);
          if (result.valid) {
            const Length lpValue =
                lpUnit.convertFromUnit(result.value);  // can throw
            if (lpValue >= s2length(minimum)) {
              res.evaluated_value = l2s(lpValue);
              res.valid = true;
            }
          }
        } catch (const Exception& e) {
        }
        return res;
      });
  b.on_format_angle([](const int& value) {
    const Angle angle = s2angle(value);
    return q2s(Toolbox::floatToString(angle.toDeg(), 3, QLocale()));
  });
  b.on_parse_angle_input([](slint::SharedString text) {
    ui::AngleEditParseResult res{false, 0};
    try {
      QString value = s2q(text);

      // Remove unit
      value.replace("°", "");

      // Parse expression and convert to Angle.
      const MathParser::Result result = MathParser().parse(value);
      if (result.valid) {
        const Angle angle = Angle::fromDeg(result.value);
        res.evaluated_value = l2s(angle);
        res.valid = true;
      }
    } catch (const Exception& e) {
    }
    return res;
  });
  b.on_format_ratio([](const int& value) {
    const Ratio ratio = s2ratio(value);
    return q2s(Toolbox::floatToString(ratio.toPercent(), 3, QLocale()));
  });
  b.on_parse_ratio_input(
      [](slint::SharedString text, int minimum, int maximum) {
        ui::RatioEditParseResult res{false, 0};
        try {
          QString value = s2q(text);

          // Remove unit and spaces
          value.replace("%", "");
          value.replace(" ", "");

          // Parse expression and convert to Ratio.
          const MathParser::Result result = MathParser().parse(value);
          if (result.valid) {
            const Ratio ratio = Ratio::fromPercent(result.value);
            if ((ratio >= s2ratio(minimum)) && (ratio <= s2ratio(maximum))) {
              res.evaluated_value = l2s(ratio);
              res.valid = true;
            }
          }
        } catch (const Exception& e) {
        }
        return res;
      });

  // Build wrapper.
  auto mw = std::make_shared<MainWindow>(*this, win, id);
  bind(mw.get(), d, &ui::Data::set_local_libraries_data, mLocalLibraries.get(),
       &LibrariesModel::uiDataChanged, mLocalLibraries->getUiData());
  bind(mw.get(), d, &ui::Data::set_remote_libraries_data,
       mRemoteLibraries.get(), &LibrariesModel::uiDataChanged,
       mRemoteLibraries->getUiData());
  bind(mw.get(), d, &ui::Data::set_libraries_panel_filter,
       mLibrariesFilter.get(), &SlintKeyEventTextBuilder::textChanged,
       mLibrariesFilter->getText());
  bind(mw.get(), d, &ui::Data::set_libraries_rescan_in_progress,
       &mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanInProgressChanged,
       mWorkspace.getLibraryDb().isScanInProgress());
  bind(mw.get(), d, &ui::Data::set_workspace_contains_standard_components, this,
       &GuiApplication::librariesContainStandardComponentsChanged,
       mLibrariesContainStandardComponents);
  connect(this, &GuiApplication::statusBarMessageChanged, mw.get(),
          &MainWindow::showStatusBarMessage);
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

int GuiApplication::getWindowCount() const noexcept {
  return mWindows.count();
}

void GuiApplication::stopWindowStateAutosaveTimer() noexcept {
  mSaveOpenedWindowsCountdown.stop();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GuiApplication::exec() {
  slint::run_event_loop();
}

void GuiApplication::quit(QPointer<QWidget> parent) noexcept {
  // Need to be delayed since this call is made from the object to be
  // deleted.
  QMetaObject::invokeMethod(
      this,
      [this, parent]() {
        if (requestClosingAllProjects() && requestClosingAllLibraries()) {
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
    openProject(filepath, qApp->activeWindow());
  } else if (!silent) {
    qWarning() << "Ignore invalid request to open project:" << file;
  }
}

void GuiApplication::openProjectLibraryUpdater(
    const FilePath& project) noexcept {
  bool wasOpen = false;
  for (auto prj : *mProjects) {
    if (prj->getProject().getFilepath() == project) {
      wasOpen = true;
      break;
    }
  }
  mProjectLibraryUpdater.reset(new ProjectLibraryUpdater(
      mWorkspace, project, [this](const FilePath& fp) {
        for (int i = 0; i < mProjects->count(); ++i) {
          if (mProjects->at(i)->getProject().getFilepath() == fp) {
            closeProject(i);
            break;
          }
        }
        return true;
      }));
  if (wasOpen) {
    connect(
        mProjectLibraryUpdater.get(), &ProjectLibraryUpdater::finished, this,
        [this](const FilePath& fp) { openProject(fp, qApp->activeWindow()); });
  }
  mProjectLibraryUpdater->show();
}

void GuiApplication::highlightErcMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool zoomTo,
    int windowId) noexcept {
  ProjectEditor* prjEditor = dynamic_cast<ProjectEditor*>(sender());
  if (!prjEditor) {
    qCritical() << "Signal from unknown ProjectEditor.";
    return;
  }
  if (auto win = getWindowById(windowId)) {
    win->highlightErcMessage(*prjEditor, msg, zoomTo);
  } else {
    qCritical() << "Unknown window ID:" << windowId;
  }
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

void GuiApplication::updateLibrariesContainStandardComponents() noexcept {
  bool found = false;
  try {
    // Check only the resistor component, that should be enough...
    const FilePath fp = mWorkspace.getLibraryDb().getLatest<Component>(
        Uuid::fromString("ef80cd5e-2689-47ee-8888-31d04fc99174"));
    found = fp.isValid();
  } catch (const Exception& e) {
  }
  if (found != mLibrariesContainStandardComponents) {
    mLibrariesContainStandardComponents = found;
    emit librariesContainStandardComponentsChanged(found);
  }
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

void GuiApplication::moveTab(int srcWindowId, int srcSectionIndex,
                             int srcTabIndex, int dstWindowId,
                             int dstSectionIndex, int dstTabIndex,
                             bool forceSwitchToTab) noexcept {
  if ((srcWindowId == dstWindowId) && (srcSectionIndex == dstSectionIndex) &&
      (dstTabIndex > srcTabIndex)) {
    --dstTabIndex;  // Moving to the right needs index correction.
  }
  if ((srcWindowId == dstWindowId) && (srcSectionIndex == dstSectionIndex) &&
      (dstTabIndex == srcTabIndex)) {
    return;  // Tab is actually not moved (destination == source).
  }
  if ((srcSectionIndex == 0) && (srcTabIndex == 0)) {
    return;  // Home tab is not movable.
  }
  auto srcWindow = getWindowById(srcWindowId);
  auto dstWindow = getWindowById(dstWindowId);
  if (srcWindow && dstWindow) {
    bool wasCurrentTab = false;
    bool wasCurrentSection = false;
    if (auto tab = srcWindow->removeTab(srcSectionIndex, srcTabIndex,
                                        &wasCurrentTab, &wasCurrentSection)) {
      if (dstTabIndex == -1) {
        dstWindow->addSection(dstSectionIndex, true);
        dstTabIndex = 0;
      }
      if ((dstSectionIndex == 0) && (dstTabIndex == 0)) {
        dstTabIndex = 1;  // Index 0 is the home tab.
      }
      const bool switchToTab = forceSwitchToTab ||
          (wasCurrentTab &&
           (wasCurrentSection || (dstWindowId != srcWindowId) ||
            (dstSectionIndex != srcSectionIndex)));
      const bool switchToSection = (wasCurrentSection && wasCurrentTab);
      dstWindow->addTab(tab, dstSectionIndex, dstTabIndex, switchToTab,
                        switchToSection);
    }
  }
}

std::shared_ptr<MainWindow> GuiApplication::getWindowById(int id) noexcept {
  for (const auto& win : mWindows) {
    if (win->getId() == id) {
      return win;
    }
  }
  return nullptr;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
