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
#include <librepcb/core/project/project.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/editor/workspace/desktopservices.h>
#include "project/projectsmodel.h"
#include <QtCore>
#include "project/projecteditor.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int index,
                       QObject* parent) noexcept
  : QObject(parent), mApp(app), mWindow(win), mIndex(index) {
  // Register global callbacks.
  const ui::Globals& globals = mWindow->global<ui::Globals>();
  globals.set_current_project(ui::ProjectData{});
  globals.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));

  mWindow->show();
}

MainWindow::~MainWindow() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void MainWindow::projectItemDoubleClicked(
    const slint::SharedString& path) noexcept {
  const FilePath fp(s2q(path));
  if (!fp.isValid()) {
    qWarning() << "Invalid file path:" << path.data();
    return;
  }
  if ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz")) {
    mProject = mApp.getProjects().openProject(fp);

    const ui::Globals& globals = mWindow->global<ui::Globals>();
    globals.set_current_project(ui::ProjectData{
                                  true,
                                  q2s(*mProject->getProject().getName()),
                                });
    mWindow->set_page(ui::MainPage::Project);
  } else {
    DesktopServices ds(mApp.getWorkspace().getSettings(), nullptr);
    ds.openLocalPath(fp);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
