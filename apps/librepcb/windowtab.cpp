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
#include "windowtab.h"

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

WindowTab::WindowTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                     ui::TabType type, int objIndex, const QString& title,
                     QObject* parent) noexcept
  : QObject(parent),
    mUiData{type, q2s(title)},
    mProject(prj),
    mObjIndex(objIndex),
    mLayerProvider(new DefaultGraphicsLayerProvider(
        app.getWorkspace().getSettings().themes.getActive())),
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            applyProjection(mAnimationDataStart.interpolated(
                mAnimationDataDelta, value.toReal()));
          });
}

WindowTab::~WindowTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowTab::activate() noexcept {
  if (mUiData.type == ui::TabType::Schematic) {
    if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
      mOpenGlSceneBuilder.reset();
      mOpenGlView.reset();
      mScene.reset(new SchematicGraphicsScene(
          *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
          this));
      mUiData.overlay_color = q2s(Qt::black);
      emit requestRepaint();
    }
  } else if (mUiData.type == ui::TabType::Board2d) {
    if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
      mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
      connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
              [this](BoardPlaneFragmentsBuilder::Result result) {
                if (result.applyToBoard()) {
                  emit requestRepaint();
                }
              });
      mPlaneBuilder->start(*brd);
      mOpenGlSceneBuilder.reset();
      mOpenGlView.reset();
      mScene.reset(new BoardGraphicsScene(
          *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
          this));
      mUiData.overlay_color = q2s(Qt::white);
      emit requestRepaint();
    }
  } else if (mUiData.type == ui::TabType::Board3d) {
    if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
      mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
      connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
              [this](BoardPlaneFragmentsBuilder::Result result) {
                if (result.applyToBoard()) {
                  emit requestRepaint();
                }
              });
      mPlaneBuilder->start(*brd);
      mScene.reset();
      mOpenGlView.reset(new OpenGlView());
      mOpenGlView->setTransform(mProjection.transform, mProjection.fov,
                                mProjection.center);
      mOpenGlSceneBuilder.reset(new OpenGlSceneBuilder(this));
      connect(mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded,
              mOpenGlView.get(), &OpenGlView::addObject);
      connect(
          mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded, this,
          [this]() { emit requestRepaint(); }, Qt::QueuedConnection);
      auto av =
          mProject->getProject().getCircuit().getAssemblyVariants().value(0);
      mOpenGlSceneBuilder->start(brd->buildScene3D(
          av ? std::make_optional(av->getUuid()) : std::nullopt));
      mUiData.overlay_color = q2s(Qt::black);
      emit requestRepaint();
    }
  }
}

void WindowTab::deactivate() noexcept {
  mPlaneBuilder.reset();
  mScene.reset();
  mOpenGlView.reset();
  mOpenGlSceneBuilder.reset();
}

slint::Image WindowTab::renderScene(float width, float height) noexcept {
  if (mScene) {
    QPixmap pixmap(width, height);
    pixmap.fill(dynamic_cast<BoardGraphicsScene*>(mScene.get()) ? Qt::black
                                                                : Qt::white);
    {
      QPainter painter(&pixmap);
      painter.setRenderHints(QPainter::Antialiasing |
                             QPainter::SmoothPixmapTransform);
      QRectF targetRect(0, 0, width, height);
      QRectF sceneRect;
      if (mProjection.scale == 0) {
        sceneRect = mScene->itemsBoundingRect();
        mProjection.scale = std::min(targetRect.width() / sceneRect.width(),
                                     targetRect.height() / sceneRect.height());
        mProjection.offset =
            sceneRect.center() - targetRect.center() / mProjection.scale;
      }
      sceneRect =
          QRectF(0, 0, width / mProjection.scale, height / mProjection.scale);
      sceneRect.translate(mProjection.offset);
      mScene->render(&painter, targetRect, sceneRect);
    }
    return q2s(pixmap);
  } else if (mOpenGlView) {
    mOpenGlView->resize(width, height);
    return q2s(mOpenGlView->grab());
  }
  return slint::Image();
}

bool WindowTab::processScenePointerEvent(
    float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  Q_UNUSED(width);
  Q_UNUSED(height);
  Projection projection = mProjection;
  if (mScene) {
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
  } else if (mOpenGlView) {
    if (e.kind == slint::private_api::PointerEventKind::Down) {
      mousePressPosition = QPoint(x, y);
      mousePressTransform = projection.transform;
      mousePressCenter = projection.center;
      buttons.insert(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Up) {
      buttons.remove(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Move) {
      const QPointF posNorm = mOpenGlView->toNormalizedPos(QPointF(x, y));
      const QPointF mousePressPosNorm =
          mOpenGlView->toNormalizedPos(mousePressPosition);

      if (buttons.contains(slint::private_api::PointerEventButton::Middle) ||
          buttons.contains(slint::private_api::PointerEventButton::Right)) {
        const QPointF cursorPosOld = mOpenGlView->toModelPos(mousePressPosNorm);
        const QPointF cursorPosNew = mOpenGlView->toModelPos(posNorm);
        projection.center = mousePressCenter + cursorPosNew - cursorPosOld;
      }
      if (buttons.contains(slint::private_api::PointerEventButton::Left)) {
        projection.transform = mousePressTransform;
        if (e.modifiers.shift) {
          // Rotate around Z axis.
          const QPointF p1 =
              mOpenGlView->toModelPos(mousePressPosNorm) - projection.center;
          const QPointF p2 =
              mOpenGlView->toModelPos(posNorm) - projection.center;
          const qreal angle1 = std::atan2(p1.y(), p1.x());
          const qreal angle2 = std::atan2(p2.y(), p2.x());
          const Angle angle = Angle::fromRad(angle2 - angle1).mappedTo180deg();
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
  return applyProjection(projection);
}

bool WindowTab::processSceneScrolled(
    float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  const qreal factor = qPow(1.3, e.delta_y / qreal(120));
  return zoom(QPointF(x, y), QSizeF(width, height), factor);
}

void WindowTab::zoomFit(float width, float height) noexcept {
  Projection projection = mProjection;
  if (mScene) {
    QRectF targetRect(0, 0, width, height);
    const QRectF sceneRect = mScene->itemsBoundingRect();
    projection.scale = std::min(targetRect.width() / sceneRect.width(),
                                targetRect.height() / sceneRect.height());
    projection.offset =
        sceneRect.center() - targetRect.center() / projection.scale;
  } else if (mOpenGlView) {
    projection.fov = sInitialFov;
    projection.center = QPointF();
    projection.transform = QMatrix4x4();
  }
  smoothTo(projection);
}

void WindowTab::zoomIn(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1.3);
}

void WindowTab::zoomOut(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1 / 1.3);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool WindowTab::zoom(const QPointF& center, const QSizeF& size,
                     qreal factor) noexcept {
  Q_UNUSED(size);
  Projection projection = mProjection;
  if (mScene) {
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
  } else if (mOpenGlView) {
    const QPointF centerNormalized = mOpenGlView->toNormalizedPos(center);
    const QPointF modelPosOld = mOpenGlView->toModelPos(centerNormalized);

    projection.fov = qBound(qreal(0.01), projection.fov / factor, qreal(90));
    mOpenGlView->setTransform(projection.transform, projection.fov,
                              projection.center);
    const QPointF modelPosNew = mOpenGlView->toModelPos(centerNormalized);
    projection.center += modelPosNew - modelPosOld;
  }
  return applyProjection(projection);
}

void WindowTab::smoothTo(const Projection& projection) noexcept {
  mAnimationDataStart = mProjection;
  mAnimationDataDelta = projection - mProjection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool WindowTab::applyProjection(const Projection& projection) noexcept {
  if (projection != mProjection) {
    mProjection = projection;
    if (mOpenGlView) {
      mOpenGlView->setTransform(projection.transform, projection.fov,
                                projection.center);
    }
    // mUiData.frame++;
    // emit uiDataChanged(mUiData.index);
    emit requestRepaint();
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
