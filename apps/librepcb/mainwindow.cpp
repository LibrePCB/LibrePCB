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

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int index,
                       QObject* parent) noexcept
  : QObject(parent),
    mIndex(index),
    mApp(app),
    mSections(new WindowSectionsModel(app, this)),
    mWindow(win) {
  // Set global data.
  const ui::Data& d = mWindow->global<ui::Data>();
  d.set_current_page(ui::MainPage::Home);
  d.set_sections(mSections);
  d.set_cursor_coordinates(slint::SharedString());

  // Bind global data to signals.
  connect(mSections.get(), &WindowSectionsModel::currentSectionIndexChanged,
          this, [&d](int index) { d.set_current_section_index(index); });
  connect(mSections.get(), &WindowSectionsModel::currentProjectChanged, this,
          &MainWindow::setCurrentProject);
  connect(
      mSections.get(), &WindowSectionsModel::cursorCoordinatesChanged, this,
      [&d](const Point& pos) {
        d.set_cursor_coordinates(q2s(QString("X: %1 Y: %2")
                                         .arg(pos.getX().toMm(), 7, 'f', 3)
                                         .arg(pos.getY().toMm(), 7, 'f', 3)));
      });

  // Register global callbacks.
  const ui::Backend& b = mWindow->global<ui::Backend>();
  b.on_action_triggered(std::bind(&MainWindow::actionTriggered, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
  b.on_key_pressed(
      std::bind(&MainWindow::keyPressed, this, std::placeholders::_1));
  b.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));
  b.on_schematic_clicked([this](int index) {
    if (auto prj = getCurrentProject()) {
      mSections->openSchematic(prj, index);
    }
  });
  b.on_board_clicked([this](int index) {
    if (auto prj = getCurrentProject()) {
      mSections->openBoard(prj, index);
    }
  });
  b.on_tab_clicked(std::bind(&WindowSectionsModel::setCurrentTab,
                             mSections.get(), std::placeholders::_1,
                             std::placeholders::_2));
  b.on_tab_close_clicked(std::bind(&WindowSectionsModel::closeTab,
                                   mSections.get(), std::placeholders::_1,
                                   std::placeholders::_2));
  b.on_render_scene(std::bind(
      &WindowSectionsModel::renderScene, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  const slint::Point<float>& scenePos,
                                  slint::private_api::PointerEvent e) {
    // const auto winPos = mWindow->window().position();
    QPointF globalPos(scenePos.x + x, scenePos.y + y);
    if (QWidget* win = qApp->activeWindow()) {
      globalPos = win->mapToGlobal(globalPos);
    }
    return mSections->processScenePointerEvent(sectionIndex, QPointF(x, y),
                                               globalPos, e);
  });
  b.on_scene_scrolled(std::bind(&WindowSectionsModel::processSceneScrolled,
                                mSections.get(), std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3,
                                std::placeholders::_4));
  b.on_scene_zoom_fit_clicked(std::bind(
      &WindowSectionsModel::zoomFit, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_in_clicked(std::bind(
      &WindowSectionsModel::zoomIn, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_out_clicked(std::bind(
      &WindowSectionsModel::zoomOut, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_open_url([this](const slint::SharedString& url) {
    DesktopServices ds(mApp.getWorkspace().getSettings());
    ds.openUrl(QUrl(s2q(url)));
  });

  // Show window.
  mWindow->show();
}

MainWindow::~MainWindow() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool MainWindow::actionTriggered(ui::ActionId id, int sectionIndex) noexcept {
  if (mSections->actionTriggered(id, sectionIndex)) {
    return true;
  } else if (id == ui::ActionId::WindowClose) {
    mWindow->hide();  // TODO: Remove from GuiApplication
    return true;
  } else if (mApp.actionTriggered(id, sectionIndex)) {
    return true;
  }

  qWarning() << "Unhandled UI action:" << static_cast<int>(id);
  return false;
}

slint::private_api::EventResult MainWindow::keyPressed(
    const slint::private_api::KeyEvent& e) noexcept {
  if ((std::string_view(e.text) == "f") && (e.modifiers.control)) {
    mWindow->invoke_focus_search();
    return slint::private_api::EventResult::Accept;
  }

  qDebug() << "Unhandled UI key event:" << e.text.data();
  return slint::private_api::EventResult::Reject;
}

void MainWindow::projectItemDoubleClicked(
    const slint::SharedString& path) noexcept {
  const FilePath fp(s2q(path));
  if (!fp.isValid()) {
    qWarning() << "Invalid file path:" << path.data();
    return;
  }
  if ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz")) {
    setCurrentProject(mApp.getProjects().openProject(fp));
    mWindow->global<ui::Data>().set_current_page(ui::MainPage::Project);
  } else {
    DesktopServices ds(mApp.getWorkspace().getSettings());
    ds.openLocalPath(fp);
  }
}

void MainWindow::setCurrentProject(
    std::shared_ptr<ProjectEditor> prj) noexcept {
  if (prj) {
    mWindow->global<ui::Data>().set_current_project_index(
        mApp.getProjects().getIndexOf(prj));
  }
}

std::shared_ptr<ProjectEditor> MainWindow::getCurrentProject() noexcept {
  return mApp.getProjects().getProject(
      mWindow->global<ui::Data>().get_current_project_index());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
