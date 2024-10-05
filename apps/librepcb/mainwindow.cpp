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

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/editor/project/boardeditor/boardgraphicsscene.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>
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
    mApp(app),
    mWindow(win),
    mGlobals(mWindow->global<ui::Globals>()),
    mIndex(index),
    mLayerProvider(new DefaultGraphicsLayerProvider(
        app.getWorkspace().getSettings().themes.getActive())),
    mTabs({
        std::make_shared<slint::VectorModel<ui::Tab>>(),
        std::make_shared<slint::VectorModel<ui::Tab>>(),
    }),
    mScenes({nullptr, nullptr}),
    mOldTransforms({{}, {}}),
    mTransforms({{}, {}}),
    mMoving({false, false}) {
  // Set initial data.
  mGlobals.set_current_project(ui::ProjectData{});
  mGlobals.set_tab_index_left(-1);
  mGlobals.set_tab_index_right(-1);

  // Register global callbacks.
  mGlobals.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));
  mGlobals.on_schematic_clicked(std::bind(&MainWindow::schematicItemClicked,
                                          this, std::placeholders::_1));
  mGlobals.on_board_clicked(
      std::bind(&MainWindow::boardItemClicked, this, std::placeholders::_1));
  mGlobals.on_tab_clicked(std::bind(&MainWindow::tabClicked, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
  mGlobals.on_tab_close_clicked(std::bind(&MainWindow::tabCloseClicked, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
  mGlobals.on_render_scene(std::bind(
      &MainWindow::renderScene, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  mGlobals.on_scene_pointer_event(std::bind(
      &MainWindow::onScnePointerEvent, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
      std::placeholders::_5, std::placeholders::_6));
  mGlobals.on_scene_scrolled(std::bind(
      &MainWindow::onSceneScrolled, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  // Set models.
  mGlobals.set_tabs_left(mTabs[0]);
  mGlobals.set_tabs_right(mTabs[1]);

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
    mProject = mApp.getProjects().openProject(fp);
    auto schematics =
        std::make_shared<slint::VectorModel<slint::SharedString>>();
    auto boards = std::make_shared<slint::VectorModel<slint::SharedString>>();

    for (auto sch : mProject->getProject().getSchematics()) {
      schematics->push_back(q2s(*sch->getName()));
    }

    for (auto brd : mProject->getProject().getBoards()) {
      boards->push_back(q2s(*brd->getName()));
    }

    mGlobals.set_current_project(ui::ProjectData{
        true,
        q2s(*mProject->getProject().getName()),
        schematics,
        boards,
    });
    mWindow->set_page(ui::MainPage::Project);
  } else {
    DesktopServices ds(mApp.getWorkspace().getSettings(), nullptr);
    ds.openLocalPath(fp);
  }
}

void MainWindow::schematicItemClicked(int index) noexcept {
  if (!mProject) return;
  auto section = (mTabs[0]->row_count() + mTabs[1]->row_count()) % 2;
  if (auto obj = mProject->getProject().getSchematicByIndex(index)) {
    mTabs[section]->push_back(
        ui::Tab{ui::TabType::Schematic, index, q2s(*obj->getName())});
    tabClicked(section, mTabs[section]->row_count() - 1);
  }
}

void MainWindow::boardItemClicked(int index) noexcept {
  if (!mProject) return;
  auto section = (mTabs[0]->row_count() + mTabs[1]->row_count()) % 2;
  if (auto obj = mProject->getProject().getBoardByIndex(index)) {
    mTabs[section]->push_back(
        ui::Tab{ui::TabType::Board, index, q2s(*obj->getName())});
    tabClicked(section, mTabs[section]->row_count() - 1);
  }
}

void MainWindow::tabClicked(int section, int index) noexcept {
  auto tabs = mTabs[section];
  auto tab = tabs ? tabs->row_data(index) : std::nullopt;
  bool success = false;
  if (tab) {
    if (tab->type == ui::TabType::Schematic) {
      if (auto sch = mProject->getProject().getSchematicByIndex(tab->index)) {
        mScenes[section].reset(new SchematicGraphicsScene(
            *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
            this));
        success = true;
      }
    } else if (tab->type == ui::TabType::Board) {
      if (auto brd = mProject->getProject().getBoardByIndex(tab->index)) {
        mScenes[section].reset(new BoardGraphicsScene(
            *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
            this));
        success = true;
      }
    }
  }
  if (success) {
    if (section == 0) {
      mGlobals.set_tab_index_left(index);
      mWindow->fn_refresh_scene_left();
    } else if (section == 1) {
      mGlobals.set_tab_index_right(index);
      mWindow->fn_refresh_scene_right();
    }
  }
}

void MainWindow::tabCloseClicked(int section, int index) noexcept {
  auto getter = std::bind((section == 1) ? &ui::Globals::get_tab_index_right
                                         : &ui::Globals::get_tab_index_left,
                          &mGlobals);

  if (auto tabs = mTabs[section]) {
    const int tabCount = static_cast<int>(tabs->row_count());
    if ((index >= 0) && (index < tabCount)) {
      tabs->erase(index);
      int currentIndex = getter();
      if (index < currentIndex) {
        --currentIndex;
      }
      tabClicked(section, std::min(currentIndex, tabCount - 2));
    }
  }
}

slint::Image MainWindow::renderScene(int section, float width, float height,
                                     int frame) noexcept {
  Q_UNUSED(frame);
  if (auto scene = mScenes[section]) {
    QPixmap pixmap(width, height);
    pixmap.fill(dynamic_cast<BoardGraphicsScene*>(scene.get()) ? Qt::black
                                                               : Qt::white);
    {
      QPainter painter(&pixmap);
      painter.setRenderHints(QPainter::Antialiasing |
                             QPainter::SmoothPixmapTransform);
      QRectF targetRect(0, 0, width, height);
      QRectF sourceRect = mTransforms[section].mapRect(targetRect);
      scene->render(&painter, targetRect, sourceRect);
    }
    return q2s(pixmap);
  } else {
    return slint::Image();
  }
}

slint::private_api::EventResult MainWindow::onScnePointerEvent(
    int section, float x1, float y1, float x0, float y0,
    slint::private_api::PointerEvent e) noexcept {
  if (e.button == slint::private_api::PointerEventButton::Left) {
    if (e.kind == slint::private_api::PointerEventKind::Down) {
      mOldTransforms[section] = mTransforms[section];
      mMoving[section] = true;
    } else if (e.kind == slint::private_api::PointerEventKind::Up) {
      mMoving[section] = false;
    }
  }
  if (mMoving[section] &&
      (e.kind == slint::private_api::PointerEventKind::Move)) {
    mTransforms[section] = mOldTransforms[section];
    mTransforms[section].translate(x0 - x1, y0 - y1);
    if (section == 0) {
      mWindow->fn_refresh_scene_left();
    } else {
      mWindow->fn_refresh_scene_right();
    }
  }
  return slint::private_api::EventResult::Accept;
}

slint::private_api::EventResult MainWindow::onSceneScrolled(
    int section, float x, float y,
    slint::private_api::PointerScrollEvent e) noexcept {
  QPointF scenePos = mTransforms[section].map(QPointF(x, y));
  qreal scaleFactor = qPow(1.3, e.delta_y / qreal(-120));
  mTransforms[section].translate(scenePos.x(), scenePos.y());
  mTransforms[section].scale(scaleFactor, scaleFactor);
  mTransforms[section].translate(-scenePos.x(), -scenePos.y());
  if (section == 0) {
    mWindow->fn_refresh_scene_left();
  } else {
    mWindow->fn_refresh_scene_right();
  }
  return slint::private_api::EventResult::Accept;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
