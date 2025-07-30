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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SlintGraphicsView::SlintGraphicsView(const QRectF& defaultSceneRect,
                                     QObject* parent) noexcept
  : QObject(parent),
    mDefaultSceneRect(defaultSceneRect),
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

void SlintGraphicsView::setUseOpenGl(bool use) noexcept {
  if (use && (!mGlContext)) {
    // Create off-screen surface.
    std::unique_ptr<QOffscreenSurface> surface(new QOffscreenSurface());
    surface->create();

    // Create OpenGL context.
    std::unique_ptr<QOpenGLContext> context(new QOpenGLContext());
    if ((!context->create()) || (!context->makeCurrent(surface.get()))) {
      mGlError = "Failed to create & activate OpenGL context.";
      return;
    }

    // Keep objects.
    mGlSurface = std::move(surface);
    mGlContext = std::move(context);
    emit transformChanged();
  } else if ((!use) && mGlSurface) {
    mGlFbo.reset();
    mGlContext.reset();
    mGlSurface.reset();
    mGlError.clear();
    emit transformChanged();
  }
}

void SlintGraphicsView::setEventHandler(
    IF_GraphicsViewEventHandler* obj) noexcept {
  mEventHandler = obj;
}

slint::Image SlintGraphicsView::render(GraphicsScene& scene, float width,
                                       float height) noexcept {
  const QSize size(qCeil(width), qCeil(height));
  if ((size.width() < 2) || (size.height() < 2)) {
    return slint::Image();
  }

  // If OpenGL is activated, enable context and prepare FBO.
  QString openGlError = mGlError;
  if (mGlSurface && mGlContext && openGlError.isEmpty()) {
    if (!mGlContext->makeCurrent(mGlSurface.get())) {
      openGlError = "Failed to make OpenGL context current.";
    }
    if (openGlError.isEmpty() && ((!mGlFbo) || (mGlFbo->size() != size))) {
      mGlFbo.reset();  // Release memory first.
      QOpenGLFramebufferObjectFormat format;
      format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
      format.setSamples(4);
      mGlFbo.reset(new QOpenGLFramebufferObject(size, format));
    }
    if (openGlError.isEmpty() && (!mGlFbo->bind())) {
      openGlError = "Failed to bind OpenGL FBO.";
    }
  }

  QPixmap pixmap(size);
  {
    std::unique_ptr<QOpenGLPaintDevice> glDev;
    if (mGlFbo && openGlError.isEmpty()) {
      glDev.reset(new QOpenGLPaintDevice(size));
    }
    QPainter painter(glDev ? static_cast<QPaintDevice*>(glDev.get())
                           : static_cast<QPaintDevice*>(&pixmap));
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    const QRectF targetRect(QPoint(0, 0), size);
    if (mViewSize.isEmpty()) {
      const QRectF initialRect = validateSceneRect(scene.itemsBoundingRect());
      mProjection.scale = std::min(targetRect.width() / initialRect.width(),
                                   targetRect.height() / initialRect.height());
      mProjection.offset =
          initialRect.center() - targetRect.center() / mProjection.scale;
    }
    QRectF sceneRect(0, 0, size.width() / mProjection.scale,
                     size.height() / mProjection.scale);
    sceneRect.translate(mProjection.offset);
    scene.render(&painter, targetRect, sceneRect);

    // If there was an OpenGL error, print it at the bottom right.
    if (!openGlError.isEmpty()) {
      painter.setPen(Qt::red);
      painter.drawText(size.width() - 5, size.height() - 5, 0, 0,
                       Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
                       openGlError);
    }

    mViewSize = targetRect.size();
  }

  // OpenGl mode: Release FBO and fetch framebuffer content.
  if (mGlFbo && openGlError.isEmpty()) {
    mGlFbo->release();
    pixmap = QPixmap::fromImage(mGlFbo->toImage());
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
 *  Static Methods
 ******************************************************************************/

static QRectF createSceneRect(qreal x, qreal y, qreal w, qreal h) noexcept {
  const qreal pxPerMm = Length::fromMm(1).toPx();
  return QRectF(x * pxPerMm, y * pxPerMm, w * pxPerMm, h * pxPerMm);
}

QRectF SlintGraphicsView::defaultSymbolSceneRect() noexcept {
  return createSceneRect(-50, -50, 100, 100);
}

QRectF SlintGraphicsView::defaultFootprintSceneRect() noexcept {
  return createSceneRect(-50, -50, 100, 100);
}

QRectF SlintGraphicsView::defaultSchematicSceneRect() noexcept {
  return createSceneRect(-20, -180, 300, 220);
}

QRectF SlintGraphicsView::defaultBoardSceneRect() noexcept {
  return createSceneRect(-20, -120, 140, 140);
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

// Helper to avoid division by zero on empty scenes.
QRectF SlintGraphicsView::validateSceneRect(const QRectF& r) const noexcept {
  return r.isEmpty() ? mDefaultSceneRect : r;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
