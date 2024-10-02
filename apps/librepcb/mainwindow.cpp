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

#include "apptoolbox.h"
#include "guiapplication.h"
#include "library/librariesmodel.h"

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

template <typename TTarget, typename TSlint, typename TClass, typename TQt>
static void bind(MainWindow* context, const TTarget& target,
                 void (TTarget::*setter)(const TSlint&) const, TClass* source,
                 void (TClass::*signal)(TQt),
                 const TSlint& defaultValue) noexcept {
  QObject::connect(source, signal, context,
                   std::bind(setter, &target, std::placeholders::_1));
  (target.*setter)(defaultValue);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindow::MainWindow(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), mApp(app), mWindow(ui::AppWindow::create()) {
  // Setup window object.
  mWindow->set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  mWindow->set_workspace_path(
      app.getWorkspace().getPath().toNative().toUtf8().data());
  mWindow->on_close([&] { slint::quit_event_loop(); });

  // Register global callbacks.
  const ui::Globals& globals = mWindow->global<ui::Globals>();
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
      std::bind(&LibrariesModel::ensurePopulated, mApp.getLibraries().get()));
  globals.set_libraries(mApp.getLibraries());
  globals.on_install_checked_libraries(std::bind(
      &LibrariesModel::installCheckedLibraries, mApp.getLibraries().get()));
  globals.on_uninstall_library(std::bind(&LibrariesModel::uninstallLibrary,
                                         mApp.getLibraries().get(),
                                         std::placeholders::_1));

  // Bind global properties.
  bind(this, globals, &ui::Globals::set_status_bar_progress,
       &app.getWorkspace().getLibraryDb(),
       &WorkspaceLibraryDb::scanProgressUpdate, 0);
  bind(this, globals, &ui::Globals::set_outdated_libraries,
       mApp.getLibraries().get(), &LibrariesModel::outdatedLibrariesChanged,
       mApp.getLibraries()->getOutdatedLibraries());
  bind(this, globals, &ui::Globals::set_checked_libraries,
       mApp.getLibraries().get(), &LibrariesModel::checkedLibrariesChanged,
       mApp.getLibraries()->getCheckedLibraries());
  bind(this, globals, &ui::Globals::set_refreshing_available_libraries,
       mApp.getLibraries().get(),
       &LibrariesModel::fetchingRemoteLibrariesChanged,
       mApp.getLibraries()->isFetchingRemoteLibraries());

  // Show window.
  mWindow->show();
}

MainWindow::~MainWindow() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void MainWindow::menuItemTriggered(ui::MenuItemId id) noexcept {
  switch (id) {
    case ui::MenuItemId::NewWindow:
      mApp.newWindow();
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
