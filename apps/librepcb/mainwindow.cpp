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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindow::MainWindow(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), mApp(app), mWindow(ui::AppWindow::create()) {
  mWindow->set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  mWindow->set_workspace_path(
      app.getWorkspace().getPath().toNative().toUtf8().data());

  QObject::connect(&app.getWorkspace().getLibraryDb(),
                   &WorkspaceLibraryDb::scanProgressUpdate, this,
                   [this](int progress) {
                     mWindow->set_status_progress(progress / qreal(100));
                   });

  mWindow->global<ui::Globals>().on_menu_item_triggered(
      [this](ui::MenuItemId id) { menuItemTriggered(id); });

  mWindow->global<ui::Globals>().set_installed_libraries(
      mApp.getInstalledLibraries());
  mWindow->global<ui::Globals>().set_available_libraries(
      mApp.getAvailableLibraries());

  mWindow->global<ui::Globals>().on_parse_length_input(
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

  mWindow->on_close([&] { slint::quit_event_loop(); });

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
