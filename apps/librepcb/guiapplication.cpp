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

#include "apptoolbox.h"
#include "library/librariesmodel.h"
#include "mainwindow.h"
#include "project/projecteditor.h"
#include "project/projectsmodel.h"
#include "workspace/favoriteprojectsmodel.h"
#include "workspace/filesystemmodel.h"
#include "workspace/recentprojectsmodel.h"

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/application.h>
#include <librepcb/core/systeminfo.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/editor/utils/standardeditorcommandhandler.h>
#include <librepcb/editor/workspace/workspacesettingsdialog.h>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GuiApplication::GuiApplication(Workspace& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mRecentProjects(new RecentProjectsModel(ws, this)),
    mFavoriteProjects(new FavoriteProjectsModel(ws, this)),
    mLibraries(new LibrariesModel(ws, this)),
    mProjects(new ProjectsModel(ws, this)) {
  mWorkspace.getLibraryDb().startLibraryRescan();
  createNewWindow();
}

GuiApplication::~GuiApplication() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool GuiApplication::actionTriggered(ui::ActionId id,
                                     int sectionIndex) noexcept {
  Q_UNUSED(sectionIndex);

  StandardEditorCommandHandler stdHandler(mWorkspace.getSettings(),
                                          qApp->activeWindow());

  if (id == ui::ActionId::OpenWorkspaceSettings) {
    WorkspaceSettingsDialog dlg(mWorkspace, qApp->activeWindow());
    dlg.exec();
    return true;
  } else if (id == ui::ActionId::OpenKeyboardShortcutsReference) {
    stdHandler.shortcutsReference();
    return true;
  } else if (id == ui::ActionId::OpenUserManual) {
    stdHandler.onlineDocumentation();
    return true;
  } else if (id == ui::ActionId::OpenSupport) {
    stdHandler.onlineSupport();
    return true;
  } else if (id == ui::ActionId::OpenDonate) {
    stdHandler.onlineDonate();
    return true;
  } else if (id == ui::ActionId::OpenWebsite) {
    stdHandler.website();
    return true;
  } else if (id == ui::ActionId::OpenSourceCode) {
    stdHandler.onlineSourceCode();
    return true;
  } else if (id == ui::ActionId::RescanWorkspaceLibraries) {
    mWorkspace.getLibraryDb().startLibraryRescan();
    return true;
  } else if (id == ui::ActionId::Quit) {
    slint::quit_event_loop();
    return true;
  } else if (id == ui::ActionId::LibraryPanelEnsurePopulated) {
    mLibraries->ensurePopulated();
    return true;
  } else if (id == ui::ActionId::LibraryPanelInstall) {
    mLibraries->installCheckedLibraries();
    return true;
  }

  return false;
}

void GuiApplication::createNewWindow(int projectIndex) noexcept {
  // Create Slint window.
  auto win = ui::AppWindow::create();

  // Set global data.
  const ui::Data& d = win->global<ui::Data>();
  d.set_preview_mode(false);
  d.set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  d.set_about_librepcb_details(q2s(buildAppVersionDetails()));
  d.set_workspace_path(mWorkspace.getPath().toNative().toUtf8().data());
  d.set_workspace_folder(std::make_shared<FileSystemModel>(
      mWorkspace, mWorkspace.getProjectsPath(), this));
  d.set_recent_projects(mRecentProjects);
  d.set_favorite_projects(mFavoriteProjects);
  d.set_libraries(mLibraries);
  d.set_projects(mProjects);
  d.set_current_project_index(projectIndex);

  // Bind global data to signals.
  bind(this, d, &ui::Data::set_status_bar_progress, &mWorkspace.getLibraryDb(),
       &WorkspaceLibraryDb::scanProgressUpdate, 0);
  bind(this, d, &ui::Data::set_outdated_libraries, mLibraries.get(),
       &LibrariesModel::outdatedLibrariesChanged,
       mLibraries->getOutdatedLibraries());
  bind(this, d, &ui::Data::set_checked_libraries, mLibraries.get(),
       &LibrariesModel::checkedLibrariesChanged,
       mLibraries->getCheckedLibraries());
  bind(this, d, &ui::Data::set_refreshing_available_libraries, mLibraries.get(),
       &LibrariesModel::fetchingRemoteLibrariesChanged,
       mLibraries->isFetchingRemoteLibraries());

  // Register global callbacks.
  const ui::Backend& b = win->global<ui::Backend>();
  b.on_parse_length_input(
      [](slint::SharedString text, slint::SharedString unit) {
        ui::EditParseResult res{false, text, unit};
        try {
          QString value = text.begin();
          foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
            foreach (const QString& suffix, unit.getUserInputSuffixes()) {
              if (value.endsWith(suffix)) {
                value.chop(suffix.length());
                res.evaluated_unit = unit.toShortStringTr().toStdString();
              }
            }
          }
          Length l = Length::fromMm(value);
          value = l.toMmString();
          if (value.endsWith(".0")) {
            value.chop(2);
          }
          res.evaluated_value = value.toStdString();
          res.valid = true;
        } catch (const Exception& e) {
        }
        return res;
      });
  b.on_open_library(std::bind(&LibrariesModel::openLibrary, mLibraries.get(),
                              std::placeholders::_1));
  b.on_uninstall_library(std::bind(&LibrariesModel::uninstallLibrary,
                                   mLibraries.get(), std::placeholders::_1));
  b.on_toggle_libraries_checked(std::bind(
      &LibrariesModel::toggleAll, mLibraries.get(), std::placeholders::_1));

  // Build wrapper.
  auto mw = std::make_shared<MainWindow>(*this, win, mWindows.count(), this);
  mWindows.append(mw);
}

void GuiApplication::exec() {
  slint::run_event_loop();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString GuiApplication::buildAppVersionDetails() const noexcept {
  // Always English, not translatable!
  QStringList details;
  const QString date = Application::getBuildDate().toString(Qt::ISODate);
  QString qt = QString(qVersion()) + " (built against " + QT_VERSION_STR + ")";
  details << "LibrePCB Version: " + Application::getVersion();
  details << "Git Revision:     " + Application::getGitRevision();
  details << "Build Date:       " + date;
  if (!Application::getBuildAuthor().isEmpty()) {
    details << "Build Author:     " + Application::getBuildAuthor();
  }
  details << "Qt Version:       " + qt;
  details << "CPU Architecture: " + QSysInfo::currentCpuArchitecture();
  details << "Operating System: " + QSysInfo::prettyProductName();
  details << "Platform Plugin:  " + qApp->platformName();
  details << "TLS Library:      " + QSslSocket::sslLibraryVersionString();
  details << "OCC Library:      " + OccModel::getOccVersionString();
  if (!SystemInfo::detectRuntime().isEmpty()) {
    details << "Runtime:          " + SystemInfo::detectRuntime();
  }
  return details.join("\n");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
