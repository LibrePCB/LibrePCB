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
#include "graphicsscenetab.h"

#include "guiapplication.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/types/point.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/editor/graphics/graphicsscene.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsSceneTab::GraphicsSceneTab(GuiApplication& app,
                                   std::shared_ptr<ProjectEditor2> prj,
                                   int objIndex, QObject* parent) noexcept
  : WindowTab(app, prj, objIndex, parent),
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

GraphicsSceneTab::~GraphicsSceneTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

slint::Image GraphicsSceneTab::renderScene(float width, float height) noexcept {
  if (mScene) {
    QPixmap pixmap(width, height);
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
  }
  return slint::Image();
}

bool GraphicsSceneTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  Q_UNUSED(globalPos);

  if (!mScene) {
    return false;
  }

  bool eventConsumed = false;

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(pos);
  mMouseEvent.scenePos = Point::fromPx(scenePosPx);

  using slint::private_api::PointerEventButton;
  using slint::private_api::PointerEventKind;

  mMouseEventIsDoubleClick = false;
  if (e.kind == PointerEventKind::Down) {
    mMouseEvent.buttons.setFlag(s2q(e.button), true);
    if (e.button == PointerEventButton::Left) {
      mMouseEventIsDoubleClick =
          (!mLeftMouseButtonDoubleClickTimer.hasExpired()) &&
          (pos == mLeftMouseButtonDownPos);
      mLeftMouseButtonDoubleClickTimer.setRemainingTime(500);
      mLeftMouseButtonDownPos = pos;
      mMouseEvent.downPos = mMouseEvent.scenePos;
    }
  } else if (e.kind == PointerEventKind::Up) {
    mMouseEvent.buttons.setFlag(s2q(e.button), false);
  }
  mMouseEvent.modifiers = s2q(e.modifiers);

  if ((e.button == PointerEventButton::Middle) ||
      (e.button == PointerEventButton::Right)) {
    if ((!mPanning) && e.kind == PointerEventKind::Down) {
      mPanningStartScreenPos = pos;
      mPanningStartScenePos = scenePosPx;
      mPanning = true;
      eventConsumed = true;
    } else if (mPanning && (e.kind == PointerEventKind::Up)) {
      mPanning = false;
      eventConsumed = (pos != mPanningStartScreenPos);  // TODO: Not so nice.
    }
  } else if (e.kind == PointerEventKind::Move) {
    if (mPanning) {
      Projection projection = mProjection;
      projection.offset -= scenePosPx - mPanningStartScenePos;
      applyProjection(projection);
      eventConsumed = true;
    }

    if (auto unit = getCurrentUnit()) {
      emit cursorCoordinatesChanged(mMouseEvent.scenePos, *unit);
    }
  }

  return eventConsumed;
}

bool GraphicsSceneTab::processSceneScrolled(
    float x, float y, slint::private_api::PointerScrollEvent e) noexcept {
  const qreal factor = qPow(1.3, e.delta_y / qreal(120));
  return zoom(QPointF(x, y), factor);
}

void GraphicsSceneTab::zoomFit(float width, float height) noexcept {
  Projection projection = mProjection;
  if (mScene) {
    QRectF targetRect(0, 0, width, height);
    const QRectF sceneRect = mScene->itemsBoundingRect();
    projection.scale = std::min(targetRect.width() / sceneRect.width(),
                                targetRect.height() / sceneRect.height());
    projection.offset =
        sceneRect.center() - targetRect.center() / projection.scale;
  }
  smoothTo(projection);
}

void GraphicsSceneTab::zoomIn(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), 1.3);
}

void GraphicsSceneTab::zoomOut(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), 1 / 1.3);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

QPainterPath GraphicsSceneTab::calcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  qreal tolerance = 5 * multiplier;  // Screen pixel tolerance.
  tolerance /= mProjection.scale;  // Scene pixel tolerance.

  QPainterPath path;
  path.addEllipse(pos.toPxQPointF(), tolerance, tolerance);
  return path;
}

Point GraphicsSceneTab::mapToScenePos(const QPointF& pos) const noexcept {
  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  return Point::fromPx(tf.map(pos));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool GraphicsSceneTab::zoom(const QPointF& center, qreal factor) noexcept {
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
  }
  return applyProjection(projection);
}

void GraphicsSceneTab::smoothTo(const Projection& projection) noexcept {
  mAnimationDataStart = mProjection;
  mAnimationDataDelta = projection - mProjection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool GraphicsSceneTab::applyProjection(const Projection& projection) noexcept {
  if (projection != mProjection) {
    mProjection = projection;
    emit requestRepaint();
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
