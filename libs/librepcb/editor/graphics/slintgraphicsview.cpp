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

#include <algorithm>
#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static const qreal sScrollFactor = 0.07;

static const QPainter::RenderHints sRenderHints =
    QPainter::Antialiasing | QPainter::SmoothPixmapTransform;

// Maps scene coordinates to a view of the given width, optionally mirrored.
static QTransform viewTransform(qreal scale, const QPointF& offset, bool mirror,
                                int width) noexcept {
  QTransform tf;
  if (mirror) {
    tf.translate(width, 0);
    tf.scale(-scale, scale);
  } else {
    tf.scale(scale, scale);
  }
  tf.translate(-offset.x(), -offset.y());
  return tf;
}

// Limits of the surfaces kept for rendering, both by count and total size.
static const int sMaxFbos = 32;
static const qint64 sMaxFboPixels = 12 * 1000 * 1000;

static void paintContent(QPaintDevice* device, GraphicsScene& scene,
                         const QSize& size,
                         const QTransform& sceneToDevice) noexcept {
  QPainter painter(device);
  painter.setRenderHints(sRenderHints);
  scene.renderContent(&painter, size, sceneToDevice);
}

static std::unique_ptr<QOpenGLFramebufferObject> createFbo(const QSize& size) {
  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  format.setSamples(4);
  return std::make_unique<QOpenGLFramebufferObject>(size, format);
}

static qreal boundedScaleFactor(qreal scale) noexcept {
  // Limit zoom factor to avoid crashes due to numerical issues when zooming
  // extensively (e.g. possible with free running scroll wheels).
  return qBound(qreal(0.001), scale, qreal(5000));
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SlintGraphicsView::SlintGraphicsView(const QRectF& defaultSceneRect,
                                     const QMarginsF& defaultMargins,
                                     QObject* parent) noexcept
  : QObject(parent),
    mDefaultSceneRect(defaultSceneRect),
    mDefaultMargins(defaultMargins),
    mEventHandler(nullptr),
    mRenderCacheEnabled(false),
    mRenderCache(new RenderCache(
        [this](GraphicsScene& scene, const QTransform& sceneToDevice,
               const QSize& size) {
          return renderContentSurface(scene, sceneToDevice, size);
        },
        [this]() {
          // The owner renders again in response to this signal.
          emit transformChanged();
        })),
    mMirror(false),
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

Point SlintGraphicsView::mapToScenePos(const QPointF& pos,
                                       qreal devicePixelRatio) const noexcept {
  return Point::fromPx(mapToScenePosPx(pos, devicePixelRatio));
}

QPointF SlintGraphicsView::mapToScenePosPx(
    QPointF pos, qreal devicePixelRatio) const noexcept {
  if (mMirror && (mViewSize.width() > 0)) {
    pos.setX(mViewSize.width() - pos.x());
  }
  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  return tf.map(pos * devicePixelRatio);
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
    invalidateRenderCache();
    emit transformChanged();
  } else if ((!use) && mGlSurface) {
    invalidateRenderCache();
    mGlContext->makeCurrent(mGlSurface.get());
    mFbos.clear();
    mGlContext.reset();
    mGlSurface.reset();
    mGlError.clear();
    emit transformChanged();
  }
}

void SlintGraphicsView::setRenderCacheEnabled(bool enable) noexcept {
  bool ok = false;
  const int envValue = qEnvironmentVariableIntValue("LPCB_RENDER_CACHE", &ok);
  mRenderCacheEnabled = enable && (!(ok && (envValue == 0)));
  if (!mRenderCacheEnabled) {
    invalidateRenderCache();
  }
}

void SlintGraphicsView::releaseResources() noexcept {
  if (mGlSurface && mGlContext && mGlError.isEmpty()) {
    mGlContext->makeCurrent(mGlSurface.get());
  }
  invalidateRenderCache();
  mFbos.clear();
}

void SlintGraphicsView::setEventHandler(
    IF_GraphicsViewEventHandler* obj) noexcept {
  mEventHandler = obj;
}

void SlintGraphicsView::setMirror(bool mirror) noexcept {
  if (mirror != mMirror) {
    mMirror = mirror;
    emit transformChanged();
  }
}

slint::Image SlintGraphicsView::render(GraphicsScene& scene, float width,
                                       float height) noexcept {
  const QSize size(qCeil(width), qCeil(height));
  if ((size.width() < 2) || (size.height() < 2)) {
    return slint::Image();
  }

  fitProjection(scene, size);
  const qreal scale = mProjection.scale;

  slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(
      static_cast<uint32_t>(size.width()),
      static_cast<uint32_t>(size.height()));
  {
    QImage out(reinterpret_cast<uchar*>(buffer.begin()), size.width(),
               size.height(), size.width() * 4,
               QImage::Format_RGBA8888_Premultiplied);

    // Get the scene content, preferably from the cache.
    QPointF offset = mProjection.offset;
    QString error = mGlError;
    bool rendered = false;
    QPoint pos;
    if (mRenderCacheEnabled && RenderCache::snapPos(offset * scale, pos)) {
      const QTransform sceneToCache =
          QTransform::fromScale(mMirror ? -scale : scale, scale);
      const QRect viewRect(
          QPoint(mMirror ? (-pos.x() - size.width()) : pos.x(), pos.y()),
          size);
      rendered = mRenderCache->render(scene, sceneToCache, viewRect,
                                      scene.getBackgroundColor(), out);
      if (rendered) {
        offset = QPointF(pos) / scale;
      }
    }
    if (!rendered) {
      const QTransform tf =
          viewTransform(scale, offset, mMirror, size.width());
      QImage content = renderContentSurface(scene, tf, size, &error);
      if (content.isNull()) {
        content = QImage(size, QImage::Format_RGBA8888_Premultiplied);
        paintContent(&content, scene, size, tf);
      }
      if (content.format() != QImage::Format_RGBA8888_Premultiplied) {
        content.convertTo(QImage::Format_RGBA8888_Premultiplied);
      }
      QPainter painter(&out);
      painter.setCompositionMode(QPainter::CompositionMode_Source);
      painter.drawImage(0, 0, content);
    }

    // The foreground layer is drawn on top of the content every frame.
    QPainter painter(&out);
    painter.setRenderHints(sRenderHints);
    scene.renderForeground(&painter, size,
                           viewTransform(scale, offset, mMirror, size.width()));

    // If there was an OpenGL error, print it at the bottom right.
    if (!error.isEmpty()) {
      painter.setPen(Qt::red);
      painter.drawText(size.width() - 5, size.height() - 5, 0, 0,
                       Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
                       error);
    }
  }
  mViewSize = size;
  return slint::Image(buffer);
}

void SlintGraphicsView::pointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  using slint::private_api::PointerEventButton;
  using slint::private_api::PointerEventKind;

  const QPointF scenePosPx = mapToScenePosPx(pos, 1);
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
        }
      } else {
        mEventHandler->graphicsSceneLeftMouseButtonPressed(mMouseEvent);
      }
    }
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Up)) {
    if (mEventHandler) {
      mEventHandler->graphicsSceneLeftMouseButtonReleased(mMouseEvent);
    }
  } else if ((e.button == PointerEventButton::Middle) &&
             (e.kind == PointerEventKind::Down)) {
    mPanningStartScreenPos = pos;
    mPanningStartScenePos = scenePosPx;
    mPanning = true;
    emit stateChanged();
  } else if ((e.button == PointerEventButton::Middle) &&
             (e.kind == PointerEventKind::Up)) {
    mPanning = false;
    emit stateChanged();
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Down)) {
    mPanningStartScreenPos = pos;
    mPanningStartScenePos = scenePosPx;
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Up)) {
    if (mPanning) {
      mPanning = false;
      emit stateChanged();
    } else if (mEventHandler) {
      mEventHandler->graphicsSceneRightMouseButtonReleased(mMouseEvent);
    }
  } else if (e.kind == PointerEventKind::Move) {
    if ((!mPanning) && (mMouseEvent.buttons.testFlag(Qt::RightButton))) {
      const QPointF d = pos - mPanningStartScreenPos;
      const qreal distance = std::sqrt(d.x() * d.x() + d.y() * d.y());
      if (distance > 5) {
        mPanning = true;
        emit stateChanged();
      } else {
        return;
      }
    }
    if (mPanning) {
      Projection projection = mProjection;
      projection.autoFitInView = false;
      projection.offset -= scenePosPx - mPanningStartScenePos;
      applyProjection(projection);
    } else if (mEventHandler) {
      mEventHandler->graphicsSceneMouseMoved(mMouseEvent);
    }
  }
}

bool SlintGraphicsView::scrollEvent(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  if (std::abs(e.delta_x) > std::abs(e.delta_y)) {
    // Horizontal scrolling.
    scroll(QPointF(-e.delta_x / mProjection.scale, 0));
    return true;
  } else if (e.modifiers.shift) {
    // Vertical scrolling with SHIFT -> scroll horizontally.
    scroll(QPointF(-e.delta_y / mProjection.scale, 0));
    return true;
  } else if (e.modifiers.control) {
    // Vertical scrolling with CTRL -> scroll vertically.
    scroll(QPointF(0, -e.delta_y / mProjection.scale));
    return true;
  } else {
    // Vertical scrolling -> zoom in/out.
    zoom(pos, qPow(1.3, e.delta_y / qreal(120)));
    return true;
  }
}

bool SlintGraphicsView::keyPressed(
    const slint::language::KeyEvent& e) noexcept {
  if (!mEventHandler) return false;

  if (const Qt::Key key = s2key(e.text)) {
    return mEventHandler->graphicsSceneKeyPressed(
        GraphicsSceneKeyEvent{key, s2q(e.modifiers)});
  }
  return false;
}

bool SlintGraphicsView::keyReleased(
    const slint::language::KeyEvent& e) noexcept {
  if (!mEventHandler) return false;

  if (const Qt::Key key = s2key(e.text)) {
    return mEventHandler->graphicsSceneKeyReleased(
        GraphicsSceneKeyEvent{key, s2q(e.modifiers)});
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

void SlintGraphicsView::zoomToSceneRect(const QRectF& r,
                                        bool autoFitInView) noexcept {
  const QRectF sourceRect = validateSceneRect(r);
  const QRectF targetRect =
      QRectF(QPointF(0, 0), mViewSize).marginsRemoved(mDefaultMargins);

  if ((targetRect.width() < 2) || (targetRect.height() < 2)) {
    return;
  }

  Projection projection;
  projection.autoFitInView = autoFitInView;
  projection.scale =
      boundedScaleFactor(std::min(targetRect.width() / sourceRect.width(),
                                  targetRect.height() / sourceRect.height()));
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

QMarginsF SlintGraphicsView::defaultMargins() noexcept {
  return QMarginsF(5, 5, 5, 5);
}

QMarginsF SlintGraphicsView::defaultEditorMargins() noexcept {
  return QMarginsF(40, 35, 40, 25);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QImage SlintGraphicsView::renderContentSurface(
    GraphicsScene& scene, const QTransform& sceneToDevice, const QSize& size,
    QString* error) noexcept {
  if (mGlSurface && mGlContext) {
    QString glError = mGlError;
    if (glError.isEmpty() && (!mGlContext->makeCurrent(mGlSurface.get()))) {
      glError = "Failed to make OpenGL context current.";
    }
    std::shared_ptr<QOpenGLFramebufferObject> fbo;
    if (glError.isEmpty()) {
      fbo = acquireFbo(size);
      if (!fbo->bind()) {
        glError = "Failed to bind OpenGL FBO.";
      }
    }
    if (!glError.isEmpty()) {
      if (error) {
        *error = glError;
      }
      return QImage();
    }
    {
      QOpenGLPaintDevice device(size);
      paintContent(&device, scene, size, sceneToDevice);
    }
    fbo->release();
    return fbo->toImage();
  }

  QImage img(size, QImage::Format_RGBA8888_Premultiplied);
  paintContent(&img, scene, size, sceneToDevice);
  return img;
}

std::shared_ptr<QOpenGLFramebufferObject> SlintGraphicsView::acquireFbo(
    const QSize& size) noexcept {
  const QPair<int, int> key(size.width(), size.height());
  std::shared_ptr<QOpenGLFramebufferObject> fbo = mFbos.value(key);
  if (!fbo) {
    qint64 pixels = static_cast<qint64>(size.width()) * size.height();
    for (const auto& f : mFbos) {
      pixels += static_cast<qint64>(f->width()) * f->height();
    }
    if ((mFbos.count() >= sMaxFbos) || (pixels > sMaxFboPixels)) {
      mFbos.clear();  // Release memory first.
    }
    fbo = createFbo(size);
    mFbos.insert(key, fbo);
  }
  return fbo;
}

void SlintGraphicsView::invalidateRenderCache() noexcept {
  mRenderCache->invalidate();
}

void SlintGraphicsView::fitProjection(GraphicsScene& scene,
                                      const QSize& size) noexcept {
  const QRectF targetRect(QPoint(0, 0), size);
  if ((mViewSize != targetRect.size()) &&
      (mProjection.autoFitInView || mViewSize.isEmpty())) {
    const QRectF target = targetRect.marginsRemoved(mDefaultMargins);
    const QRectF initialRect = validateSceneRect(scene.itemsBoundingRect());
    mProjection.scale =
        boundedScaleFactor(std::min(target.width() / initialRect.width(),
                                    target.height() / initialRect.height()));
    mProjection.offset =
        initialRect.center() - target.center() / mProjection.scale;
  }
}

void SlintGraphicsView::scroll(const QPointF& delta) noexcept {
  Projection projection = mProjection;
  projection.autoFitInView = false;
  projection.offset += delta;
  applyProjection(projection);
}

void SlintGraphicsView::zoom(QPointF center, qreal factor) noexcept {
  if (mMirror && (mViewSize.width() > 0)) {
    center.setX(mViewSize.width() - center.x());
  }

  Projection projection = mProjection;
  projection.autoFitInView = false;

  QTransform tf;
  tf.translate(projection.offset.x(), projection.offset.y());
  tf.scale(1 / projection.scale, 1 / projection.scale);
  const QPointF scenePos0 = tf.map(center);
  projection.scale = boundedScaleFactor(projection.scale * factor);

  QTransform tf2;
  tf2.translate(projection.offset.x(), projection.offset.y());
  tf2.scale(1 / projection.scale, 1 / projection.scale);
  const QPointF scenePos2 = tf2.map(center);
  projection.offset -= scenePos2 - scenePos0;

  applyProjection(projection);
}

void SlintGraphicsView::smoothTo(const Projection& projection) noexcept {
  // Do not start an animation if we are already at the desired projection.
  // This can happen for example when locating the same DRC message on a board
  // multiple times consecutively. The animation is not only useless then,
  // but also disturbes the user if he immediately starts to drag/zoom manually.
  if (projection == mProjection) {
    return;
  }

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
