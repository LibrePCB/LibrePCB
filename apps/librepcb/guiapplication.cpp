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

#include <librepcb/core/application.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

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
    mProjects(new ProjectsModel(this)) {
  mWorkspace.getLibraryDb().startLibraryRescan();
  createNewWindow();
}

GuiApplication::~GuiApplication() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GuiApplication::exec() {
  slint::run_event_loop();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GuiApplication::createNewWindow() noexcept {
  // Create Slint window.
  auto win = ui::AppWindow::create();
  win->set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  win->set_workspace_path(mWorkspace.getPath().toNative().toUtf8().data());
  win->on_close([&] { slint::quit_event_loop(); });

  // Register global callbacks.
  const ui::Globals& globals = win->global<ui::Globals>();
  globals.on_menu_item_triggered(
      [this](ui::MenuItemId id) { menuItemTriggered(id); });
  globals.on_parse_length_input(
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
  globals.on_ensure_libraries_populated(
      std::bind(&LibrariesModel::ensurePopulated, mLibraries.get()));
  globals.on_install_checked_libraries(
      std::bind(&LibrariesModel::installCheckedLibraries, mLibraries.get()));
  globals.on_uninstall_library(std::bind(&LibrariesModel::uninstallLibrary,
                                         mLibraries.get(),
                                         std::placeholders::_1));

  // Set models.
  globals.set_workspace_folder(std::make_shared<FileSystemModel>(
      mWorkspace, mWorkspace.getProjectsPath(), this));
  globals.set_recent_projects(mRecentProjects);
  globals.set_favorite_projects(mFavoriteProjects);
  globals.set_libraries(mLibraries);
  globals.set_open_projects(mProjects);

  // Bind global properties.
  bind(this, globals, &ui::Globals::set_status_bar_progress,
       &mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanProgressUpdate, 0);
  bind(this, globals, &ui::Globals::set_outdated_libraries, mLibraries.get(),
       &LibrariesModel::outdatedLibrariesChanged,
       mLibraries->getOutdatedLibraries());
  bind(this, globals, &ui::Globals::set_checked_libraries, mLibraries.get(),
       &LibrariesModel::checkedLibrariesChanged,
       mLibraries->getCheckedLibraries());
  bind(this, globals, &ui::Globals::set_refreshing_available_libraries,
       mLibraries.get(), &LibrariesModel::fetchingRemoteLibrariesChanged,
       mLibraries->isFetchingRemoteLibraries());

  // Build wrapper.
  mWindows.append(
      std::make_shared<MainWindow>(*this, win, mWindows.count(), this));
}

void GuiApplication::menuItemTriggered(ui::MenuItemId id) noexcept {
  switch (id) {
    case ui::MenuItemId::NewWindow:
      createNewWindow();
      break;
    default:
      qWarning() << "Unknown menu item triggered:" << static_cast<int>(id);
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
