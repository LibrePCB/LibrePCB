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

#include "apptoolbox.h"
#include "guiapplication.h"

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
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsSceneTab::GraphicsSceneTab(GuiApplication& app, ui::TabType type,
                                   std::shared_ptr<ProjectEditor> prj,
                                   int objIndex, const QString& title,
                                   const QColor& bgColor,
                                   QObject* parent) noexcept
  : WindowTab(app, type, prj, objIndex, title, parent),
    mBackgroundColor(bgColor),
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
    pixmap.fill(mBackgroundColor);
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
        mStartScenePos = scenePosPx;
        mPanning = true;
      } else if (e.kind == slint::private_api::PointerEventKind::Up) {
        mPanning = false;
      }
    }
    if (mPanning && (e.kind == slint::private_api::PointerEventKind::Move)) {
      projection.offset -= scenePosPx - mStartScenePos;
    }
    const Point scenePos = Point::fromPx(scenePosPx);
    emit cursorCoordinatesChanged(scenePos.getX().toMm(),
                                  scenePos.getY().toMm());
  }
  return applyProjection(projection);
}

bool GraphicsSceneTab::processSceneScrolled(
    float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  const qreal factor = qPow(1.3, e.delta_y / qreal(120));
  return zoom(QPointF(x, y), QSizeF(width, height), factor);
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
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1.3);
}

void GraphicsSceneTab::zoomOut(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), QSizeF(width, height), 1 / 1.3);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool GraphicsSceneTab::zoom(const QPointF& center, const QSizeF& size,
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

}  // namespace app
}  // namespace editor
}  // namespace librepcb
