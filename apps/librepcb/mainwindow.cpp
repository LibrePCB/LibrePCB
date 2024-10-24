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
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/3d/openglscenebuilder.h>
#include <librepcb/editor/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/editor/project/boardeditor/boardgraphicsscene.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>
#include <librepcb/editor/widgets/openglview.h>
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
    mPlaneBuilder(new BoardPlaneFragmentsBuilder(false, this)),
    mLayerProvider(new DefaultGraphicsLayerProvider(
        app.getWorkspace().getSettings().themes.getActive())),
    mSectionsData(new slint::VectorModel<ui::SectionData>()) {
  // Set initial data.
  mGlobals.set_current_project(ui::ProjectData{});
  mWindow->set_cursor_coordinate(slint::SharedString());

  // Register global callbacks.
  mGlobals.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));
  mGlobals.on_schematic_clicked(std::bind(&MainWindow::schematicItemClicked,
                                          this, std::placeholders::_1));
  mGlobals.on_board_clicked(
      std::bind(&MainWindow::boardItemClicked, this, std::placeholders::_1));
  mGlobals.on_board_3d_clicked(std::bind(&MainWindow::board3dItemClicked, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
  mGlobals.on_tab_clicked(std::bind(&MainWindow::tabClicked, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
  mGlobals.on_tab_close_clicked(std::bind(&MainWindow::tabCloseClicked, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
  mGlobals.on_render_scene(
      std::bind(&MainWindow::renderScene, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5));
  mGlobals.on_scene_pointer_event(
      std::bind(&MainWindow::onScenePointerEvent, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5));
  mGlobals.on_scene_scrolled(
      std::bind(&MainWindow::onSceneScrolled, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5));

  // Set models.
  mGlobals.set_sections(mSectionsData);

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
  if (auto obj = mProject->getProject().getSchematicByIndex(index)) {
    addTab(ui::TabType::Schematic, *obj->getName(), index);
  }
}

void MainWindow::boardItemClicked(int index) noexcept {
  if (!mProject) return;
  if (auto obj = mProject->getProject().getBoardByIndex(index)) {
    addTab(ui::TabType::Board2d, *obj->getName(), index);
  }
}

void MainWindow::board3dItemClicked(int section, int tab) noexcept {
  if (section >= mSections.count()) return;
  if (tab >= mSections[section].tabs.count()) return;
  auto tabObj = mSections[section].tabs[tab];
  if (auto obj =
          tabObj.project->getProject().getBoardByIndex(tabObj.objIndex)) {
    addTab(ui::TabType::Board3d, *obj->getName(), tabObj.objIndex);
  }
}

void MainWindow::addTab(ui::TabType type, const QString& title,
                        int objIndex) noexcept {
  int section = 0;
  if (mSections.count() < 2) {
    mSections.append(Section{});
    mSectionsData->push_back(ui::SectionData{
        static_cast<int>(mSections.count() - 1),
        std::make_shared<slint::VectorModel<ui::Tab>>(),
        0,
        slint::Brush(),
        0,
    });
    section = mSections.count() - 1;
  } else {
    for (std::size_t i = 0; i < mSectionsData->row_count(); ++i) {
      section += mSectionsData->row_data(i)->tabs->row_count();
    }
    section %= 2;
  }

  mSections[section].tabs.append(Tab{
      mProject,
      type,
      objIndex,
      QPointF(),
      1,
      QRectF(),
  });
  auto tabs = std::dynamic_pointer_cast<slint::VectorModel<ui::Tab>>(
      mSectionsData->row_data(section)->tabs);
  tabs->push_back(ui::Tab{type, q2s(title)});
  tabClicked(section, tabs->row_count() - 1);
}

void MainWindow::tabClicked(int section, int tab) noexcept {
  if (section >= mSections.count()) return;
  auto& sectionObj = mSections[section];
  if (tab >= sectionObj.tabs.count()) return;
  auto& tabObj = mSections[section].tabs[tab];
  auto sectionData = mSectionsData->row_data(section);
  if (!sectionData) return;

  bool success = false;
  if (tabObj.type == ui::TabType::Schematic) {
    if (auto sch =
            tabObj.project->getProject().getSchematicByIndex(tabObj.objIndex)) {
      sectionObj.openGlSceneBuilder.reset();
      sectionObj.openGlView.reset();
      sectionObj.scene.reset(new SchematicGraphicsScene(
          *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
          this));
      sectionData->overlay_color = q2s(Qt::black);
      success = true;
    }
  } else if (tabObj.type == ui::TabType::Board2d) {
    if (auto brd =
            tabObj.project->getProject().getBoardByIndex(tabObj.objIndex)) {
      mPlaneBuilder->startAsynchronously(*brd);
      sectionObj.openGlSceneBuilder.reset();
      sectionObj.openGlView.reset();
      sectionObj.scene.reset(new BoardGraphicsScene(
          *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
          this));
      sectionData->overlay_color = q2s(Qt::white);
      success = true;
    }
  } else if (tabObj.type == ui::TabType::Board3d) {
    if (auto brd =
            tabObj.project->getProject().getBoardByIndex(tabObj.objIndex)) {
      mPlaneBuilder->startAsynchronously(*brd);
      sectionObj.scene.reset();
      sectionObj.openGlView.reset(new OpenGlView());
      sectionObj.openGlSceneBuilder.reset(new OpenGlSceneBuilder(this));
      connect(sectionObj.openGlSceneBuilder.get(),
              &OpenGlSceneBuilder::objectAdded, sectionObj.openGlView.get(),
              &OpenGlView::addObject);
      connect(
          sectionObj.openGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded,
          this, [this, section]() { mWindow->fn_refresh_scene(section); },
          Qt::QueuedConnection);
      sectionObj.openGlSceneBuilder->start(brd->buildScene3D(tl::nullopt));
      sectionData->overlay_color = q2s(Qt::black);
      success = true;
    }
  }
  if (success) {
    sectionData->tab_index = tab;
    mSectionsData->set_row_data(section, *sectionData);
    mWindow->fn_refresh_scene(section);
  }
}

void MainWindow::tabCloseClicked(int section, int tab) noexcept {
  if (section >= mSections.count()) return;
  auto& sectionObj = mSections[section];
  auto sectionData = mSectionsData->row_data(section);
  if (!sectionData) return;
  auto tabs =
      std::dynamic_pointer_cast<slint::VectorModel<ui::Tab>>(sectionData->tabs);

  const int tabCount = static_cast<int>(tabs->row_count());
  if (tabCount == 1) {
    mSectionsData->erase(section);
    mSections.remove(section);
    for (std::size_t i = section; i < mSectionsData->row_count(); ++i) {
      auto data = mSectionsData->row_data(i);
      data->index--;
      mSectionsData->set_row_data(i, *data);
    }
  } else if ((tab >= 0) && (tab < tabCount)) {
    sectionObj.tabs.remove(tab);
    tabs->erase(tab);
    int currentIndex = sectionData->tab_index;
    if (tab < currentIndex) {
      --currentIndex;
    }
    tabClicked(section, std::min(currentIndex, tabCount - 2));
  }
}

slint::Image MainWindow::renderScene(int section, int tab, float width,
                                     float height, int frame) noexcept {
  Q_UNUSED(frame);

  if (section >= mSections.count()) return slint::Image();
  auto& sectionObj = mSections[section];
  if (tab >= sectionObj.tabs.count()) return slint::Image();
  auto& tabObj = sectionObj.tabs[tab];

  if (auto scene = sectionObj.scene) {
    QPixmap pixmap(width, height);
    pixmap.fill(dynamic_cast<BoardGraphicsScene*>(scene.get()) ? Qt::black
                                                               : Qt::white);
    {
      QPainter painter(&pixmap);
      painter.setRenderHints(QPainter::Antialiasing |
                             QPainter::SmoothPixmapTransform);
      QRectF targetRect(0, 0, width, height);
      if (tabObj.sceneRect.isEmpty()) {
        const QRectF sceneRect = scene->itemsBoundingRect();
        tabObj.scale = std::min(targetRect.width() / sceneRect.width(),
                                targetRect.height() / sceneRect.height());
        tabObj.offset = sceneRect.center() - targetRect.center() / tabObj.scale;
      }
      tabObj.sceneRect =
          QRectF(0, 0, width / tabObj.scale, height / tabObj.scale);
      tabObj.sceneRect.translate(tabObj.offset);
      scene->render(&painter, targetRect, tabObj.sceneRect);
    }
    return q2s(pixmap);
  } else if (auto view = sectionObj.openGlView) {
    view->resize(width, height);
    return q2s(view->grab());
  } else {
    return slint::Image();
  }
}

slint::private_api::EventResult MainWindow::onScenePointerEvent(
    int section, int tab, float x, float y,
    slint::private_api::PointerEvent e) noexcept {
  if (section >= mSections.count()) {
    return slint::private_api::EventResult::Reject;
  }
  auto& sectionObj = mSections[section];
  if (tab >= sectionObj.tabs.count()) {
    return slint::private_api::EventResult::Reject;
  }
  auto& tabObj = sectionObj.tabs[tab];

  if (e.kind == slint::private_api::PointerEventKind::Down) {
    mGlobals.set_current_section(section);
  }

  if (sectionObj.scene) {
    QTransform t;
    t.translate(tabObj.offset.x(), tabObj.offset.y());
    t.scale(1 / tabObj.scale, 1 / tabObj.scale);
    QPointF scenePosPx = t.map(QPointF(x, y));

    if ((e.button == slint::private_api::PointerEventButton::Middle) ||
        (e.button == slint::private_api::PointerEventButton::Right)) {
      if (e.kind == slint::private_api::PointerEventKind::Down) {
        sectionObj.startScenePos = scenePosPx;
        sectionObj.panning = true;
      } else if (e.kind == slint::private_api::PointerEventKind::Up) {
        sectionObj.panning = false;
      }
    }
    if (sectionObj.panning &&
        (e.kind == slint::private_api::PointerEventKind::Move)) {
      tabObj.offset -= scenePosPx - sectionObj.startScenePos;
      mWindow->fn_refresh_scene(section);
    }
    const Point scenePos = Point::fromPx(scenePosPx);
    mWindow->set_cursor_coordinate(q2s(QString("%1, %2")
                                           .arg(scenePos.getX().toMm())
                                           .arg(scenePos.getY().toMm())));
  } else if (auto view = sectionObj.openGlView) {
    if (e.kind == slint::private_api::PointerEventKind::Down) {
      sectionObj.mousePressPosition = QPoint(x, y);
      sectionObj.mousePressTransform = tabObj.transform;
      sectionObj.mousePressCenter = tabObj.projectionCenter;
      sectionObj.buttons.insert(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Up) {
      sectionObj.buttons.remove(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Move) {
      const QPointF posNorm = view->toNormalizedPos(QPointF(x, y));
      const QPointF mousePressPosNorm =
          view->toNormalizedPos(sectionObj.mousePressPosition);

      if (sectionObj.buttons.contains(
              slint::private_api::PointerEventButton::Middle) ||
          sectionObj.buttons.contains(
              slint::private_api::PointerEventButton::Right)) {
        const QPointF cursorPosOld = view->toModelPos(mousePressPosNorm);
        const QPointF cursorPosNew = view->toModelPos(posNorm);
        tabObj.projectionCenter =
            sectionObj.mousePressCenter + cursorPosNew - cursorPosOld;
        view->setTransform(tabObj.transform, tabObj.projectionFov,
                           tabObj.projectionCenter);
        mWindow->fn_refresh_scene(section);
      }
      if (sectionObj.buttons.contains(
              slint::private_api::PointerEventButton::Left)) {
        tabObj.transform = sectionObj.mousePressTransform;
        if (e.modifiers.shift) {
          // Rotate around Z axis.
          const QPointF p1 =
              view->toModelPos(mousePressPosNorm) - tabObj.projectionCenter;
          const QPointF p2 =
              view->toModelPos(posNorm) - tabObj.projectionCenter;
          const qreal angle1 = std::atan2(p1.y(), p1.x());
          const qreal angle2 = std::atan2(p2.y(), p2.x());
          const Angle angle = Angle::fromRad(angle2 - angle1).mappedTo180deg();
          const QVector3D axis = sectionObj.mousePressTransform.inverted().map(
              QVector3D(0, 0, angle.toDeg()));
          tabObj.transform.rotate(QQuaternion::fromAxisAndAngle(
              axis.normalized(), angle.abs().toDeg()));
        } else {
          // Rotate around X/Y axis.
          const QVector2D delta(posNorm - mousePressPosNorm);
          const QVector3D axis = sectionObj.mousePressTransform.inverted().map(
              QVector3D(-delta.y(), delta.x(), 0));
          tabObj.transform.rotate(QQuaternion::fromAxisAndAngle(
              axis.normalized(), delta.length() * 270));
        }
        view->setTransform(tabObj.transform, tabObj.projectionFov,
                           tabObj.projectionCenter);
        mWindow->fn_refresh_scene(section);
      }
    }
  }

  return slint::private_api::EventResult::Accept;
}

slint::private_api::EventResult MainWindow::onSceneScrolled(
    int section, int tab, float x, float y,
    slint::private_api::PointerScrollEvent e) noexcept {
  if (section >= mSections.count()) {
    return slint::private_api::EventResult::Reject;
  }
  auto& sectionObj = mSections[section];
  if (tab >= sectionObj.tabs.count()) {
    return slint::private_api::EventResult::Reject;
  }
  auto& tabObj = sectionObj.tabs[tab];

  qreal factor = qPow(1.3, e.delta_y / qreal(120));

  if (auto view = sectionObj.openGlView) {
    const QPointF centerNormalized = view->toNormalizedPos(QPointF(x, y));
    const QPointF modelPosOld = view->toModelPos(centerNormalized);
    tabObj.projectionFov =
        qBound(qreal(0.01), tabObj.projectionFov / factor, qreal(90));
    const QPointF modelPosNew = view->toModelPos(centerNormalized);
    tabObj.projectionCenter += modelPosNew - modelPosOld;
    view->setTransform(tabObj.transform, tabObj.projectionFov,
                       tabObj.projectionCenter);
  } else {
    QTransform t;
    t.translate(tabObj.offset.x(), tabObj.offset.y());
    t.scale(1 / tabObj.scale, 1 / tabObj.scale);
    QPointF scenePos0 = t.map(QPointF(x, y));

    tabObj.scale *= factor;

    QTransform t2;
    t2.translate(tabObj.offset.x(), tabObj.offset.y());
    t2.scale(1 / tabObj.scale, 1 / tabObj.scale);
    QPointF scenePos2 = t2.map(QPointF(x, y));

    tabObj.offset -= scenePos2 - scenePos0;
  }

  mWindow->fn_refresh_scene(section);
  return slint::private_api::EventResult::Accept;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
