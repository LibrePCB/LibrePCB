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
#include "windowsection.h"

#include "apptoolbox.h"
#include "guiapplication.h"
#include "project/projecteditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/3d/openglscenebuilder.h>
#include <librepcb/editor/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/editor/project/boardeditor/boardgraphicsscene.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>
#include <librepcb/editor/widgets/openglview.h>

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

WindowSection::WindowSection(GuiApplication& app, int index,
                             QObject* parent) noexcept
  : QObject(parent),
    mUiData{index, std::make_shared<slint::VectorModel<ui::Tab>>(), -1,
            slint::Brush(), 0},
    mLayerProvider(new DefaultGraphicsLayerProvider(
        app.getWorkspace().getSettings().themes.getActive())),
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            if (Tab* t = getTab(mUiData.tab_index)) {
              applyProjection(*t,
                              mAnimationDataStart.interpolated(
                                  mAnimationDataDelta, value.toReal()));
            }
          });
}

WindowSection::~WindowSection() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowSection::setIndex(int index) noexcept {
  mUiData.index = index;
}

std::shared_ptr<ProjectEditor> WindowSection::getCurrentProject() noexcept {
  if (Tab* t = getTab(mUiData.tab_index)) {
    return t->project;
  }
  return nullptr;
}

void WindowSection::addTab(std::shared_ptr<ProjectEditor> prj, ui::TabType type,
                           int objIndex, const QString& title) noexcept {
  Tab t;
  t.project = prj;
  t.type = type;
  t.objIndex = objIndex;
  tabs.append(t);

  auto uiTabs =
      std::dynamic_pointer_cast<slint::VectorModel<ui::Tab>>(mUiData.tabs);
  uiTabs->push_back(ui::Tab{type, q2s(title)});
}

void WindowSection::closeTab(int index) noexcept {
  auto uiTabs =
      std::dynamic_pointer_cast<slint::VectorModel<ui::Tab>>(mUiData.tabs);
  const int tabCount = static_cast<int>(uiTabs->row_count());
  if ((index >= 0) && (index < tabCount)) {
    tabs.remove(index);
    uiTabs->erase(index);
    int currentIndex = mUiData.tab_index;
    if (index < currentIndex) {
      --currentIndex;
    }
    setCurrentTab(std::min(currentIndex, tabCount - 2));
  }
}

void WindowSection::setCurrentTab(int index) noexcept {
  if (Tab* t = getTab(index)) {
    if (index == mUiData.tab_index) {
      return;  // No change.
    }

    if (t->type == ui::TabType::Schematic) {
      if (auto sch =
              t->project->getProject().getSchematicByIndex(t->objIndex)) {
        openGlSceneBuilder.reset();
        openGlView.reset();
        scene.reset(new SchematicGraphicsScene(
            *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
            this));
        mUiData.overlay_color = q2s(Qt::black);
        mUiData.frame++;
      }
    } else if (t->type == ui::TabType::Board2d) {
      if (auto brd = t->project->getProject().getBoardByIndex(t->objIndex)) {
        mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
        connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished,
                this, [this](BoardPlaneFragmentsBuilder::Result result) {
                  if (result.applyToBoard()) {
                    mUiData.frame++;
                    emit uiDataChanged(mUiData.index);
                  }
                });
        mPlaneBuilder->start(*brd);
        openGlSceneBuilder.reset();
        openGlView.reset();
        scene.reset(new BoardGraphicsScene(
            *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
            this));
        mUiData.overlay_color = q2s(Qt::white);
        mUiData.frame++;
      }
    } else if (t->type == ui::TabType::Board3d) {
      if (auto brd = t->project->getProject().getBoardByIndex(t->objIndex)) {
        mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
        connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished,
                this, [this](BoardPlaneFragmentsBuilder::Result result) {
                  if (result.applyToBoard()) {
                    mUiData.frame++;
                    emit uiDataChanged(mUiData.index);
                  }
                });
        mPlaneBuilder->start(*brd);
        scene.reset();
        openGlView.reset(new OpenGlView());
        openGlView->setTransform(t->projection.transform, t->projection.fov,
                                 t->projection.center);
        openGlSceneBuilder.reset(new OpenGlSceneBuilder(this));
        connect(openGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded,
                openGlView.get(), &OpenGlView::addObject);
        connect(
            openGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded, this,
            [this]() {
              mUiData.frame++;
              emit uiDataChanged(mUiData.index);
            },
            Qt::QueuedConnection);
        auto av =
            t->project->getProject().getCircuit().getAssemblyVariants().value(
                0);
        openGlSceneBuilder->start(brd->buildScene3D(
            av ? std::make_optional(av->getUuid()) : std::nullopt));
        mUiData.overlay_color = q2s(Qt::black);
        mUiData.frame++;
      }
    }

    emit currentProjectChanged(t->project);
  }

  mUiData.tab_index = index;
  emit uiDataChanged(mUiData.index);
}

slint::Image WindowSection::renderScene(int tab, float width,
                                        float height) noexcept {
  if (Tab* t = getTab(tab)) {
    if (scene) {
      QPixmap pixmap(width, height);
      pixmap.fill(dynamic_cast<BoardGraphicsScene*>(scene.get()) ? Qt::black
                                                                 : Qt::white);
      {
        QPainter painter(&pixmap);
        painter.setRenderHints(QPainter::Antialiasing |
                               QPainter::SmoothPixmapTransform);
        QRectF targetRect(0, 0, width, height);
        QRectF sceneRect;
        if (t->projection.scale == 0) {
          sceneRect = scene->itemsBoundingRect();
          t->projection.scale =
              std::min(targetRect.width() / sceneRect.width(),
                       targetRect.height() / sceneRect.height());
          t->projection.offset =
              sceneRect.center() - targetRect.center() / t->projection.scale;
        }
        sceneRect = QRectF(0, 0, width / t->projection.scale,
                           height / t->projection.scale);
        sceneRect.translate(t->projection.offset);
        scene->render(&painter, targetRect, sceneRect);
      }
      return q2s(pixmap);
    } else if (openGlView) {
      openGlView->resize(width, height);
      return q2s(openGlView->grab());
    }
  }
  return slint::Image();
}

bool WindowSection::processScenePointerEvent(
    float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  Q_UNUSED(width);
  Q_UNUSED(height);
  if (Tab* t = getTab(mUiData.tab_index)) {
    Projection projection = t->projection;
    if (scene) {
      QTransform tf;
      tf.translate(projection.offset.x(), projection.offset.y());
      tf.scale(1 / projection.scale, 1 / projection.scale);
      QPointF scenePosPx = tf.map(QPointF(x, y));

      if ((e.button == slint::private_api::PointerEventButton::Middle) ||
          (e.button == slint::private_api::PointerEventButton::Right)) {
        if (e.kind == slint::private_api::PointerEventKind::Down) {
          startScenePos = scenePosPx;
          panning = true;
        } else if (e.kind == slint::private_api::PointerEventKind::Up) {
          panning = false;
        }
      }
      if (panning && (e.kind == slint::private_api::PointerEventKind::Move)) {
        projection.offset -= scenePosPx - startScenePos;
      }
      const Point scenePos = Point::fromPx(scenePosPx);
      emit cursorCoordinatesChanged(scenePos.getX().toMm(),
                                    scenePos.getY().toMm());
    } else if (openGlView) {
      if (e.kind == slint::private_api::PointerEventKind::Down) {
        mousePressPosition = QPoint(x, y);
        mousePressTransform = projection.transform;
        mousePressCenter = projection.center;
        buttons.insert(e.button);
      } else if (e.kind == slint::private_api::PointerEventKind::Up) {
        buttons.remove(e.button);
      } else if (e.kind == slint::private_api::PointerEventKind::Move) {
        const QPointF posNorm = openGlView->toNormalizedPos(QPointF(x, y));
        const QPointF mousePressPosNorm =
            openGlView->toNormalizedPos(mousePressPosition);

        if (buttons.contains(slint::private_api::PointerEventButton::Middle) ||
            buttons.contains(slint::private_api::PointerEventButton::Right)) {
          const QPointF cursorPosOld =
              openGlView->toModelPos(mousePressPosNorm);
          const QPointF cursorPosNew = openGlView->toModelPos(posNorm);
          projection.center = mousePressCenter + cursorPosNew - cursorPosOld;
        }
        if (buttons.contains(slint::private_api::PointerEventButton::Left)) {
          projection.transform = mousePressTransform;
          if (e.modifiers.shift) {
            // Rotate around Z axis.
            const QPointF p1 =
                openGlView->toModelPos(mousePressPosNorm) - projection.center;
            const QPointF p2 =
                openGlView->toModelPos(posNorm) - projection.center;
            const qreal angle1 = std::atan2(p1.y(), p1.x());
            const qreal angle2 = std::atan2(p2.y(), p2.x());
            const Angle angle =
                Angle::fromRad(angle2 - angle1).mappedTo180deg();
            const QVector3D axis = mousePressTransform.inverted().map(
                QVector3D(0, 0, angle.toDeg()));
            projection.transform.rotate(QQuaternion::fromAxisAndAngle(
                axis.normalized(), angle.abs().toDeg()));
          } else {
            // Rotate around X/Y axis.
            const QVector2D delta(posNorm - mousePressPosNorm);
            const QVector3D axis = mousePressTransform.inverted().map(
                QVector3D(-delta.y(), delta.x(), 0));
            projection.transform.rotate(QQuaternion::fromAxisAndAngle(
                axis.normalized(), delta.length() * 270));
          }
        }
      }
    }
    return applyProjection(*t, projection);
  }

  return false;
}

bool WindowSection::processSceneScrolled(
    float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  const qreal factor = qPow(1.3, e.delta_y / qreal(120));
  return zoom(QPointF(x, y), QSizeF(width, height), factor);
}

void WindowSection::zoomFit(float width, float height) noexcept {
  if (Tab* t = getTab(mUiData.tab_index)) {
    Projection projection = t->projection;
    if (scene) {
      QRectF targetRect(0, 0, width, height);
      const QRectF sceneRect = scene->itemsBoundingRect();
      projection.scale = std::min(targetRect.width() / sceneRect.width(),
                                  targetRect.height() / sceneRect.height());
      projection.offset =
          sceneRect.center() - targetRect.center() / projection.scale;
    } else if (openGlView) {
      projection.fov = sInitialFov;
      projection.center = QPointF();
      projection.transform = QMatrix4x4();
    }
    smoothTo(*t, projection);
  }
}

void WindowSection::zoomIn(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1.3);
}

void WindowSection::zoomOut(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1 / 1.3);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool WindowSection::zoom(const QPointF& center, const QSizeF& size,
                         qreal factor) noexcept {
  Q_UNUSED(size);
  if (Tab* t = getTab(mUiData.tab_index)) {
    Projection projection = t->projection;
    if (scene) {
      QTransform tf;
      tf.translate(projection.offset.x(), projection.offset.y());
      tf.scale(1 / projection.scale, 1 / projection.scale);
      const QPointF scenePos0 = tf.map(center);

      projection.scale *= factor;

      QTransform tf2;
      tf2.translate(projection.offset.x(), projection.offset.y());
      tf2.scale(1 / projection.scale, 1 / projection.scale);
      const QPointF scenePos2 = tf2.map(center);

      projection.offset -= scenePos2 - scenePos0;
    } else if (openGlView) {
      const QPointF centerNormalized = openGlView->toNormalizedPos(center);
      const QPointF modelPosOld = openGlView->toModelPos(centerNormalized);

      projection.fov = qBound(qreal(0.01), projection.fov / factor, qreal(90));
      openGlView->setTransform(projection.transform, projection.fov,
                               projection.center);
      const QPointF modelPosNew = openGlView->toModelPos(centerNormalized);
      projection.center += modelPosNew - modelPosOld;
    }
    return applyProjection(*t, projection);
  }

  return false;
}

void WindowSection::smoothTo(Tab& tab, const Projection& projection) noexcept {
  mAnimationDataStart = tab.projection;
  mAnimationDataDelta = projection - tab.projection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool WindowSection::applyProjection(Tab& tab,
                                    const Projection& projection) noexcept {
  if (projection != tab.projection) {
    tab.projection = projection;
    if (openGlView) {
      openGlView->setTransform(projection.transform, projection.fov,
                               projection.center);
    }
    mUiData.frame++;
    emit uiDataChanged(mUiData.index);
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
