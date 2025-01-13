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
#include "project/projecteditor.h"
#include "project/projectsmodel.h"
#include "windowsectionsmodel.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/workspace/desktopservices.h>

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

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int index,
                       QObject* parent) noexcept
  : QObject(parent),
    mIndex(index),
    mApp(app),
    mSections(new WindowSectionsModel(app, this)),
    mCurrentProject(),
    mWindow(win) {
  // Set initial data.
  const ui::Globals& g = mWindow->global<ui::Globals>();
  g.set_current_project(ui::ProjectData{});
  mWindow->set_cursor_coordinate(slint::SharedString());

  // Register global callbacks.
  g.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));
  g.on_schematic_clicked([this](int index) {
    if (mCurrentProject) mSections->openSchematic(mCurrentProject, index);
  });
  g.on_board_clicked([this](int index) {
    if (mCurrentProject) mSections->openBoard(mCurrentProject, index);
  });
  g.on_board_3d_clicked(std::bind(&WindowSectionsModel::openBoard3dViewer,
                                  mSections.get(), std::placeholders::_1));
  g.on_section_split_clicked(std::bind(&WindowSectionsModel::splitSection,
                                       mSections.get(), std::placeholders::_1));
  g.on_section_close_clicked(std::bind(&WindowSectionsModel::closeSection,
                                       mSections.get(), std::placeholders::_1));
  g.on_tab_clicked(std::bind(&WindowSectionsModel::setCurrentTab,
                             mSections.get(), std::placeholders::_1,
                             std::placeholders::_2));
  g.on_tab_close_clicked(std::bind(&WindowSectionsModel::closeTab,
                                   mSections.get(), std::placeholders::_1,
                                   std::placeholders::_2));
  g.on_open_create_library_tab(
      std::bind(&WindowSectionsModel::openCreateLibraryTab, mSections.get()));
  g.on_create_library(std::bind(&WindowSectionsModel::createLibrary,
                                mSections.get(), std::placeholders::_1));
  g.on_render_scene(std::bind(
      &WindowSectionsModel::renderScene, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  g.on_scene_pointer_event(std::bind(
      &WindowSectionsModel::processScenePointerEvent, mSections.get(),
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
      std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  g.on_scene_scrolled(std::bind(
      &WindowSectionsModel::processSceneScrolled, mSections.get(),
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
      std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  g.on_scene_zoom_fit_clicked(std::bind(
      &WindowSectionsModel::zoomFit, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  g.on_scene_zoom_in_clicked(std::bind(
      &WindowSectionsModel::zoomIn, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  g.on_scene_zoom_out_clicked(std::bind(
      &WindowSectionsModel::zoomOut, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));

  // Set models.
  g.set_sections(mSections);

  // Connect model callbacks.
  connect(mSections.get(), &WindowSectionsModel::currentSectionIndexChanged,
          this, [&g](int index) { g.set_current_section_index(index); });
  connect(mSections.get(), &WindowSectionsModel::currentProjectChanged, this,
          &MainWindow::setCurrentProject);
  connect(
      mSections.get(), &WindowSectionsModel::cursorCoordinatesChanged, this,
      [this](qreal x, qreal y) {
        mWindow->set_cursor_coordinate(q2s(QString("%1, %2").arg(x).arg(y)));
      });

  // Show window.
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
    setCurrentProject(mApp.getProjects().openProject(fp));
    mWindow->set_page(ui::MainPage::Project);
  } else {
    DesktopServices ds(mApp.getWorkspace().getSettings(), nullptr);
    ds.openLocalPath(fp);
  }
}

void MainWindow::setCurrentProject(
    std::shared_ptr<ProjectEditor> prj) noexcept {
  if (!prj) {
    return;  // Temporary workaround for disappearing schematics/boards.
  }

  if (prj != mCurrentProject) {
    mCurrentProject = prj;

    auto schematics =
        std::make_shared<slint::VectorModel<slint::SharedString>>();
    auto boards = std::make_shared<slint::VectorModel<slint::SharedString>>();
    if (mCurrentProject) {
      for (auto sch : mCurrentProject->getProject().getSchematics()) {
        schematics->push_back(q2s(*sch->getName()));
      }
      for (auto brd : mCurrentProject->getProject().getBoards()) {
        boards->push_back(q2s(*brd->getName()));
      }
    }

    const ui::Globals& g = mWindow->global<ui::Globals>();
    g.set_current_project(ui::ProjectData{
        mCurrentProject ? true : false,
        mCurrentProject ? q2s(*mCurrentProject->getProject().getName())
                        : slint::SharedString(),
        schematics,
        boards,
    });
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
