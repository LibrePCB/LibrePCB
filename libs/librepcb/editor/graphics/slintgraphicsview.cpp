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
#include "slintgraphicsview.h"

#include "../utils/slinthelpers.h"
#include "../widgets/if_graphicsvieweventhandler.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static const qreal sScrollFactor = 0.07;

// Helper to avoid division by zero on empty scenes.
static QRectF validateSceneRect(const QRectF& r) noexcept {
  if (r.isEmpty()) {
    const qreal pxPerMm = Length::fromMm(1).toPx();
    return QRectF(-20 * pxPerMm, -180 * pxPerMm, 300 * pxPerMm, 220 * pxPerMm);
  } else {
    return r;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SlintGraphicsView::SlintGraphicsView(QObject* parent) noexcept
  : QObject(parent),
    mEventHandler(nullptr),
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.get(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            applyProjection(mAnimationDataStart.interpolated(
                mAnimationDataDelta, value.toReal()));
          });
}

SlintGraphicsView::~SlintGraphicsView() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QPainterPath SlintGraphicsView::calcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  qreal tolerance = 5 * multiplier;  // Screen pixel tolerance.
  tolerance /= mProjection.scale;  // Scene pixel tolerance.

  QPainterPath path;
  path.addEllipse(pos.toPxQPointF(), tolerance, tolerance);
  return path;
}

Point SlintGraphicsView::mapToScenePos(const QPointF& pos) const noexcept {
  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  return Point::fromPx(tf.map(pos));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SlintGraphicsView::setEventHandler(
    IF_GraphicsViewEventHandler* obj) noexcept {
  mEventHandler = obj;
}

slint::Image SlintGraphicsView::render(GraphicsScene& scene, float width,
                                       float height) noexcept {
  if ((width < 2) || (height < 2)) {
    return slint::Image();
  }

  QPixmap pixmap(qCeil(width), qCeil(height));
  {
    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    const QRectF targetRect(0, 0, pixmap.width(), pixmap.height());
    if (mViewSize.isEmpty()) {
      const QRectF initialRect = validateSceneRect(scene.itemsBoundingRect());
      mProjection.scale = std::min(targetRect.width() / initialRect.width(),
                                   targetRect.height() / initialRect.height());
      mProjection.offset =
          initialRect.center() - targetRect.center() / mProjection.scale;
    }
    QRectF sceneRect(0, 0, pixmap.width() / mProjection.scale,
                     pixmap.height() / mProjection.scale);
    sceneRect.translate(mProjection.offset);
    scene.render(&painter, targetRect, sceneRect);
    mViewSize = targetRect.size();
  }
  return q2s(pixmap);
}

bool SlintGraphicsView::pointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  using slint::private_api::PointerEventButton;
  using slint::private_api::PointerEventKind;

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(pos);
  mMouseEvent.scenePos = Point::fromPx(scenePosPx);
  mMouseEvent.modifiers = s2q(e.modifiers);

  bool isDoubleClick = false;
  if (e.kind == PointerEventKind::Down) {
    mMouseEvent.buttons.setFlag(s2q(e.button), true);
    if (e.button == PointerEventButton::Left) {
      isDoubleClick = (!mLeftMouseButtonDoubleClickTimer.hasExpired()) &&
          (mMouseEvent.scenePos == mMouseEvent.downPos);
      mLeftMouseButtonDoubleClickTimer.setRemainingTime(500);
      mMouseEvent.downPos = mMouseEvent.scenePos;
    }
  } else if ((e.kind == PointerEventKind::Up) ||
             (e.kind == PointerEventKind::Cancel)) {
    mMouseEvent.buttons.setFlag(s2q(e.button), false);
  }

  if ((e.button == PointerEventButton::Left) &&
      (e.kind == PointerEventKind::Down)) {
    if (mEventHandler) {
      if (isDoubleClick) {
        if (mEventHandler->graphicsSceneLeftMouseButtonDoubleClicked(
                mMouseEvent)) {
          // Workaround for sticky button when a dialog is opened. It seems
          // we don't receive the "button up" event from Slint in that case.
          mMouseEvent.buttons.setFlag(s2q(e.button), false);
          return true;
        }
      } else {
        return mEventHandler->graphicsSceneLeftMouseButtonPressed(mMouseEvent);
      }
    }
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Up)) {
    if (mEventHandler) {
      return mEventHandler->graphicsSceneLeftMouseButtonReleased(mMouseEvent);
    }
  } else if ((e.button == PointerEventButton::Middle) &&
             (e.kind == PointerEventKind::Down)) {
    mPanningStartScreenPos = pos;
    mPanningStartScenePos = scenePosPx;
    mPanning = true;
    emit stateChanged();
    return true;
  } else if ((e.button == PointerEventButton::Middle) &&
             (e.kind == PointerEventKind::Up)) {
    mPanning = false;
    emit stateChanged();
    return true;
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Down)) {
    mPanningStartScreenPos = pos;
    mPanningStartScenePos = scenePosPx;
    return true;
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Up)) {
    if (mPanning) {
      mPanning = false;
      emit stateChanged();
      return true;
    } else if (mEventHandler) {
      return mEventHandler->graphicsSceneRightMouseButtonReleased(mMouseEvent);
    }
  } else if (e.kind == PointerEventKind::Move) {
    if ((!mPanning) && (mMouseEvent.buttons.testFlag(Qt::RightButton))) {
      const QPointF d = pos - mPanningStartScreenPos;
      const qreal distance = std::sqrt(d.x() * d.x() + d.y() * d.y());
      if (distance > 5) {
        mPanning = true;
        emit stateChanged();
      }
    }
    if (mPanning) {
      Projection projection = mProjection;
      projection.offset -= scenePosPx - mPanningStartScenePos;
      applyProjection(projection);
      return true;
    } else if (mEventHandler) {
      return mEventHandler->graphicsSceneMouseMoved(mMouseEvent);
    }
  }

  return false;
}

bool SlintGraphicsView::scrollEvent(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  if (e.modifiers.shift && (e.delta_y != 0)) {
    scroll(QPointF(-e.delta_y / mProjection.scale, 0));
    return true;
  } else if (e.modifiers.control && (e.delta_y != 0)) {
    scroll(QPointF(0, -e.delta_y / mProjection.scale));
    return true;
  } else if (e.delta_x != 0) {
    scroll(QPointF(-e.delta_x / mProjection.scale, 0));
    return true;
  } else {
    zoom(pos, qPow(1.3, e.delta_y / qreal(120)));
    return true;
  }
}

bool SlintGraphicsView::keyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  if (!mEventHandler) return false;

  const QKeySequence seq(s2q(e.text));
  if (seq.count() == 1) {
    if (e.event_type == slint::private_api::KeyEventType::KeyPressed) {
      return mEventHandler->graphicsSceneKeyPressed(
          GraphicsSceneKeyEvent{seq[0].key(), s2q(e.modifiers)});
    } else if (e.event_type == slint::private_api::KeyEventType::KeyReleased) {
      return mEventHandler->graphicsSceneKeyReleased(
          GraphicsSceneKeyEvent{seq[0].key(), s2q(e.modifiers)});
    }
  }
  return false;
}

void SlintGraphicsView::scrollLeft() noexcept {
  scroll(QPointF(-mViewSize.width() * sScrollFactor / mProjection.scale, 0));
}

void SlintGraphicsView::scrollRight() noexcept {
  scroll(QPointF(mViewSize.width() * sScrollFactor / mProjection.scale, 0));
}

void SlintGraphicsView::scrollUp() noexcept {
  scroll(QPointF(0, -mViewSize.height() * sScrollFactor / mProjection.scale));
}

void SlintGraphicsView::scrollDown() noexcept {
  scroll(QPointF(0, mViewSize.height() * sScrollFactor / mProjection.scale));
}

void SlintGraphicsView::zoomIn() noexcept {
  zoom(QPointF(mViewSize.width() / 2, mViewSize.height() / 2), 1.3);
}

void SlintGraphicsView::zoomOut() noexcept {
  zoom(QPointF(mViewSize.width() / 2, mViewSize.height() / 2), 1 / 1.3);
}

void SlintGraphicsView::zoomToSceneRect(const QRectF& r) noexcept {
  const QRectF sourceRect = validateSceneRect(r);
  const QRectF targetRect(QPointF(0, 0), mViewSize);

  if ((targetRect.width() < 2) || (targetRect.height() < 2)) {
    return;
  }

  Projection projection;
  projection.scale = std::min(targetRect.width() / sourceRect.width(),
                              targetRect.height() / sourceRect.height());
  projection.offset =
      sourceRect.center() - (targetRect.center() / projection.scale);
  smoothTo(projection);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SlintGraphicsView::scroll(const QPointF& delta) noexcept {
  Projection projection = mProjection;
  projection.offset += delta;
  applyProjection(projection);
}

void SlintGraphicsView::zoom(const QPointF& center, qreal factor) noexcept {
  Projection projection = mProjection;

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

  applyProjection(projection);
}

void SlintGraphicsView::smoothTo(const Projection& projection) noexcept {
  mAnimationDataStart = mProjection;
  mAnimationDataDelta = projection - mProjection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool SlintGraphicsView::applyProjection(const Projection& projection) noexcept {
  if (projection != mProjection) {
    mProjection = projection;
    emit transformChanged();
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
