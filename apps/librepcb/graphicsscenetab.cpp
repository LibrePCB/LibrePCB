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

GraphicsSceneTab::GraphicsSceneTab(GuiApplication& app,
                                   std::shared_ptr<ProjectEditor> prj,
                                   int objIndex, QObject* parent) noexcept
  : WindowTab(app, prj, objIndex, parent),
    mBackgroundColor(Qt::white),
    mGridColor(Qt::gray),
    mGridStyle(Theme::GridStyle::None),
    mGridInterval(2540000),
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
    // Redraw background if needed.
    if (mCachedBackground.size() != QSize(width, height)) {
      mCachedBackground = QPixmap(width, height);
      mCachedBackground.fill(mBackgroundColor);

      // Draw grid.
      if (mGridStyle != Theme::GridStyle::None) {
        QTransform tf;
        tf.translate(mProjection.offset.x(), mProjection.offset.y());
        tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
        const QTransform tfi = tf.inverted();

        const std::optional<Point> topLeft =
            Point::tryFromPx(tf.map(QPointF(0, 0)));
        const std::optional<Point> bottomRight =
            Point::tryFromPx(tf.map(QPointF(width, height)));

        if (topLeft && bottomRight) {
          const Point topLeftGrid((topLeft->getX() + (mGridInterval / 2))
                                      .mappedToGrid(*mGridInterval),
                                  (topLeft->getY() - (mGridInterval / 2))
                                      .mappedToGrid(*mGridInterval));
          const QPointF topLeftGridPx = tfi.map(topLeftGrid.toPxQPointF());
          const QPointF deltaPx =
              tfi.map(Point(*mGridInterval, -mGridInterval).toPxQPointF()) -
              tfi.map(Point(0, 0).toPxQPointF());
          if (std::min(deltaPx.x(), deltaPx.y()) > 5) {
            QPen gridPen(mGridColor);
            gridPen.setWidth((mGridStyle == Theme::GridStyle::Dots) ? 2 : 1);
            QPainter painter(&mCachedBackground);
            painter.setPen(gridPen);
            painter.setBrush(Qt::NoBrush);

            if (mGridStyle == Theme::GridStyle::Lines) {
              QVarLengthArray<QLineF, 500> lines;
              for (int i = 0; i <= static_cast<int>((width / deltaPx.x()));
                   ++i) {
                const qreal x = topLeftGridPx.x() + i * deltaPx.x();
                lines.append(QLineF(x, 0, x, height));
              }
              for (int i = 0; i <= static_cast<int>((height / deltaPx.y()));
                   ++i) {
                const qreal y = topLeftGridPx.y() + i * deltaPx.y();
                lines.append(QLineF(0, y, width, y));
              }
              painter.setOpacity(0.5);
              painter.drawLines(lines.data(), lines.size());
            } else if (mGridStyle == Theme::GridStyle::Dots) {
              QVarLengthArray<QPointF, 2000> dots;
              for (int i = 0; i <= static_cast<int>((width / deltaPx.x()));
                   ++i) {
                for (int k = 0; k <= static_cast<int>((height / deltaPx.y()));
                     ++k) {
                  const qreal x = topLeftGridPx.x() + i * deltaPx.x();
                  const qreal y = topLeftGridPx.y() + k * deltaPx.y();
                  dots.append(QPointF(x, y));
                }
              }
              painter.drawPoints(dots.data(), dots.size());
            }
          }
        }
      }
    }

    QPixmap pixmap = mCachedBackground;
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

        // Unfortunately the background we just calculated above is invalid in
        // this case. This is an ugly hack to get it redrawn...
        invalidateBackground();
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
  QPointF scenePosPx = tf.map(pos);

  if ((e.button == slint::private_api::PointerEventButton::Middle) ||
      (e.button == slint::private_api::PointerEventButton::Right)) {
    if ((!mPanning) && e.kind == slint::private_api::PointerEventKind::Down) {
      mStartScreenPos = pos;
      mStartScenePos = scenePosPx;
      mPanning = true;
      eventConsumed = true;
    } else if (mPanning &&
               (e.kind == slint::private_api::PointerEventKind::Up)) {
      mPanning = false;
      eventConsumed = (pos != mStartScreenPos);  // TODO: Not so nice.
    }
  } else if (e.kind == slint::private_api::PointerEventKind::Move) {
    if (mPanning) {
      Projection projection = mProjection;
      projection.offset -= scenePosPx - mStartScenePos;
      applyProjection(projection);
      eventConsumed = true;
    }

    const Point scenePos = Point::fromPx(scenePosPx);
    if (auto unit = getCurrentUnit()) {
      emit cursorCoordinatesChanged(scenePos, *unit);
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

Point GraphicsSceneTab::mapGlobalPosToScenePos(const QPoint& pos,
                                               bool boundToView,
                                               bool mapToGrid) const noexcept {
  return Point();  // TODO
}

void GraphicsSceneTab::invalidateBackground() noexcept {
  mCachedBackground = QPixmap();
  emit requestRepaint();
  ;
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
    mCachedBackground = QPixmap();
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
