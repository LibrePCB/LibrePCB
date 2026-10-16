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
#include <cstring>
#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static const qreal sScrollFactor = 0.07;

static qreal boundedScaleFactor(qreal scale) noexcept {
  // Limit zoom factor to avoid crashes due to numerical issues when zooming
  // extensively (e.g. possible with free running scroll wheels).
  return qBound(qreal(0.001), scale, qreal(5000));
}

static constexpr int sCacheMargin = 256;
static constexpr int sCacheQuantum = 64;

// Estimated cost model to choose between rendering separate dirty regions or
// their bounding box.
static constexpr double sDirtyRegionOverheadMs = 1.5;
static constexpr double sFullViewRenderMs = 50.0;

// Fraction of the view above which dirty regions are not worth rendering
// separately from the full view.
static constexpr double sDirtyAreaLimit = 0.8;

static int wrapIndex(int value, int size) noexcept {
  const int m = value % size;
  return (m < 0) ? (m + size) : m;
}

// Splits a rect of the wrapped device space into contiguous ring segments and
// calls f(segment relative to rect, ring position) for each of them.
template <typename F>
static void forEachRingSegment(const QRect& rect, const QSize& ring, F&& f) {
  for (int y = 0; y < rect.height();) {
    const int ry = wrapIndex(rect.y() + y, ring.height());
    const int h = std::min(rect.height() - y, ring.height() - ry);
    for (int x = 0; x < rect.width();) {
      const int rx = wrapIndex(rect.x() + x, ring.width());
      const int w = std::min(rect.width() - x, ring.width() - rx);
      f(QRect(x, y, w, h), QPoint(rx, ry));
      x += w;
    }
    y += h;
  }
}

static void copyRows(const QImage& src, const QRect& srcRect, QImage& dst,
                     const QPoint& dstPos) noexcept {
  for (int row = 0; row < srcRect.height(); ++row) {
    std::memcpy(dst.scanLine(dstPos.y() + row) + dstPos.x() * 4,
                src.constScanLine(srcRect.y() + row) + srcRect.x() * 4,
                static_cast<size_t>(srcRect.width()) * 4);
  }
}

// Parts of outer which are not covered by inner.
static QList<QRect> subtractRect(const QRect& outer,
                                 const QRect& inner) noexcept {
  const QRect i = outer.intersected(inner);
  QList<QRect> pieces;
  if (i.isEmpty()) {
    pieces.append(outer);
    return pieces;
  }
  const int x0 = outer.x(), x1 = outer.x() + outer.width();
  const int y0 = outer.y(), y1 = outer.y() + outer.height();
  const int ix0 = i.x(), ix1 = i.x() + i.width();
  const int iy0 = i.y(), iy1 = i.y() + i.height();
  if (iy0 > y0) pieces.append(QRect(x0, y0, x1 - x0, iy0 - y0));
  if (y1 > iy1) pieces.append(QRect(x0, iy1, x1 - x0, y1 - iy1));
  if (ix0 > x0) pieces.append(QRect(x0, iy0, ix0 - x0, iy1 - iy0));
  if (x1 > ix1) pieces.append(QRect(ix1, iy0, x1 - ix1, iy1 - iy0));
  return pieces;
}

// Largest rect covered by the union of two overlapping rects which contains
// the second one.
static QRect mergeValid(const QRect& old, const QRect& view) noexcept {
  QRect best = view;
  auto area = [](const QRect& r) {
    return static_cast<qint64>(r.width()) * r.height();
  };
  if ((view.top() >= old.top()) && (view.bottom() <= old.bottom())) {
    const QRect r(QPoint(std::min(old.left(), view.left()), view.top()),
                  QPoint(std::max(old.right(), view.right()), view.bottom()));
    if (area(r) > area(best)) best = r;
  }
  if ((view.left() >= old.left()) && (view.right() <= old.right())) {
    const QRect r(QPoint(view.left(), std::min(old.top(), view.top())),
                  QPoint(view.right(), std::max(old.bottom(), view.bottom())));
    if (area(r) > area(best)) best = r;
  }
  return best;
}

// Temporary statistics, enabled with LPCB_RENDER_CACHE_LOG=1.
namespace {
struct CacheStats {
  int frames = 0;
  int cold = 0;
  int partial = 0;
  int chunks = 0;
  qint64 renderNs = 0;
  qint64 composeNs = 0;
  qint64 coldNs = 0;
  qint64 partialNs = 0;
  qint64 chunkNs = 0;
  int regions = 0;
  qint64 drawNs = 0;
  qint64 readNs = 0;
  qint64 convNs = 0;
  qint64 storeNs = 0;
  int dirtyUpdates = 0;
  int dirtyRects = 0;
  int dirtyFallbacks = 0;
  qint64 dirtyNs = 0;
  int moveCalls = 0;
  qint64 moveNs = 0;
  QElapsedTimer since;

  void flush(const QRect& valid, const QSize& ring) {
    if (!since.isValid()) {
      since.start();
    }
    if (since.elapsed() < 1000) {
      return;
    }
    auto avg = [](qint64 ns, int n) { return n ? ns * 1e-6 / n : 0.0; };
    qInfo().noquote() << QString(
                             "RENDER-CACHE frames %1 (cold %2, partial %3), "
                             "avg ms: render() %4, compose %5, cold %6, "
                             "partial %7; refill chunks %8 avg %9 ms; valid "
                             "%10x%11 of %12x%13")
                             .arg(frames)
                             .arg(cold)
                             .arg(partial)
                             .arg(avg(renderNs, frames), 0, 'f', 2)
                             .arg(avg(composeNs, frames), 0, 'f', 2)
                             .arg(avg(coldNs, cold), 0, 'f', 2)
                             .arg(avg(partialNs, partial), 0, 'f', 2)
                             .arg(chunks)
                             .arg(avg(chunkNs, chunks), 0, 'f', 2)
                             .arg(valid.width())
                             .arg(valid.height())
                             .arg(ring.width())
                             .arg(ring.height());
    qInfo().noquote()
        << QString(
               "RENDER-CACHE phases per region (n=%1), avg ms: draw %2, "
               "readback %3, convert %4, store %5")
               .arg(regions)
               .arg(avg(drawNs, regions), 0, 'f', 2)
               .arg(avg(readNs, regions), 0, 'f', 2)
               .arg(avg(convNs, regions), 0, 'f', 2)
               .arg(avg(storeNs, regions), 0, 'f', 2);
    qInfo().noquote()
        << QString(
               "RENDER-CACHE dirty updates %1 (rects %2, fallbacks %3), "
               "avg ms per update: %4")
               .arg(dirtyUpdates)
               .arg(dirtyRects)
               .arg(dirtyFallbacks)
               .arg(avg(dirtyNs, dirtyUpdates), 0, 'f', 2);
    qInfo().noquote()
        << QString("RENDER-CACHE mouse-move handler calls %1, avg ms %2")
               .arg(moveCalls)
               .arg(avg(moveNs, moveCalls), 0, 'f', 2);
    *this = CacheStats();
    since.start();
  }
};
}  // namespace
static CacheStats sCacheStats;
static const bool sCacheLog =
    qEnvironmentVariableIntValue("LPCB_RENDER_CACHE_LOG") != 0;

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
    mRefillTimer(new QTimer(this)),
    mMirror(false),
    mAnimation(new QVariantAnimation(this)) {
  mRefillTimer->setSingleShot(true);
  connect(mRefillTimer.get(), &QTimer::timeout, this,
          [this]() { refillRenderCache(); });
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
    mGlFbo.reset();
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

void SlintGraphicsView::releaseRenderCache() noexcept {
  if (mGlSurface && mGlContext && mGlError.isEmpty()) {
    mGlContext->makeCurrent(mGlSurface.get());
  }
  invalidateRenderCache();
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

  if (mRenderCacheEnabled) {
    slint::Image cached;
    if (renderCached(scene, size, cached)) {
      return cached;
    }
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
      mGlFbo = std::make_unique<QOpenGLFramebufferObject>(size, format);
    }
    if (openGlError.isEmpty() && (!mGlFbo->bind())) {
      openGlError = "Failed to bind OpenGL FBO.";
    }
  }

  QPixmap pixmap(size);
  {
    std::unique_ptr<QOpenGLPaintDevice> glDev;
    if (mGlFbo && openGlError.isEmpty()) {
      glDev = std::make_unique<QOpenGLPaintDevice>(size);
    }
    QPainter painter(glDev ? static_cast<QPaintDevice*>(glDev.get())
                           : static_cast<QPaintDevice*>(&pixmap));
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    const QRectF targetRect(QPoint(0, 0), size);
    fitProjection(scene, size);
    QRectF sceneRect(0, 0, size.width() / mProjection.scale,
                     size.height() / mProjection.scale);
    sceneRect.translate(mProjection.offset);

    // Render the scene, either mirrored or not.
    if (mMirror) {
      painter.save();
      painter.translate(size.width(), 0);
      painter.scale(-1, 1);
    }
    scene.render(&painter, targetRect, sceneRect);
    if (mMirror) {
      painter.restore();
    }

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
      QElapsedTimer moveTimer;
      moveTimer.start();
      mEventHandler->graphicsSceneMouseMoved(mMouseEvent);
      if (sCacheLog) {
        sCacheStats.moveCalls++;
        sCacheStats.moveNs += moveTimer.nsecsElapsed();
      }
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

bool SlintGraphicsView::renderCached(GraphicsScene& scene, const QSize& size,
                                     slint::Image& result) noexcept {
  QElapsedTimer timer;
  timer.start();

  const bool openGl = mGlSurface && mGlContext;
  if (openGl &&
      ((!mGlError.isEmpty()) || (!mGlContext->makeCurrent(mGlSurface.get())))) {
    return false;
  }

  // Deliver pending scene change notifications to get the current changes.
  QCoreApplication::sendPostedEvents(&scene, QEvent::MetaCall);
  QList<QRectF> dirty;
  dirty = scene.takeDirtyRects();
  fitProjection(scene, size);

  const qreal scale = mProjection.scale;
  if ((qAbs(mProjection.offset.x() * scale) > 1e9) ||
      (qAbs(mProjection.offset.y() * scale) > 1e9)) {
    return false;
  }

  // Snap the offset to whole device pixels, so cached pixels stay reusable.
  const QPoint pos(qRound(mProjection.offset.x() * scale),
                   qRound(mProjection.offset.y() * scale));
  const QPointF offset(pos.x() / scale, pos.y() / scale);
  const QRect view(
      QPoint(mMirror ? (-pos.x() - size.width()) : pos.x(), pos.y()), size);
  const int margin = sCacheMargin;
  const QSize ringSize(size.width() + 2 * margin, size.height() + 2 * margin);
  const QRect window = view.adjusted(-margin, -margin, margin, margin);

  const bool match = (mCache.ring.size() == ringSize) &&
      (mCache.scale == scale) && (mCache.mirror == mMirror) &&
      (mCache.openGl == openGl) && (mCacheScene == &scene);
  if (!match) {
    if (mCache.ring.size() != ringSize) {
      mCache.ring = QImage(ringSize, QImage::Format_RGBA8888_Premultiplied);
    }
    mCache.valid = QRect();
    mCache.scale = scale;
    mCache.mirror = mMirror;
    mCache.openGl = openGl;
    mCacheScene = &scene;
  }

  bool cold = false;
  bool partial = false;
  mCache.valid = mCache.valid.intersected(window);
  mCache.view = view;
  applyDirtyRects(scene, dirty);
  if (!mCache.valid.contains(view)) {
    if (!mCache.valid.intersects(view)) {
      cold = true;
      storeRegion(view, renderRegion(scene, view));
      mCache.valid = view;
    } else {
      partial = true;
      const QRect old = mCache.valid;
      for (const QRect& piece : subtractRect(view, old)) {
        storeRegion(piece, renderRegion(scene, piece));
      }
      mCache.valid = mergeValid(old, view);
    }
  }
  const qint64 renderNs = timer.nsecsElapsed();

  // The foreground layer is drawn on top of the cached content every frame.
  slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(
      static_cast<uint32_t>(size.width()),
      static_cast<uint32_t>(size.height()));
  {
    QImage out(reinterpret_cast<uchar*>(buffer.begin()), size.width(),
               size.height(), size.width() * 4,
               QImage::Format_RGBA8888_Premultiplied);
    cropRegion(view, out);
    QPainter painter(&out);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    if (mMirror) {
      painter.translate(size.width(), 0);
      painter.scale(-1, 1);
    }
    QRectF sceneRect(0, 0, size.width() / scale, size.height() / scale);
    sceneRect.translate(offset);
    scene.renderForeground(&painter, QRectF(QPoint(0, 0), size), sceneRect);
  }
  mViewSize = size;
  result = slint::Image(buffer);

  if ((mCache.valid != window) && (!mRefillTimer->isActive())) {
    mRefillTimer->start(1);
  }

  if (sCacheLog) {
    sCacheStats.frames++;
    sCacheStats.renderNs += renderNs;
    sCacheStats.composeNs += timer.nsecsElapsed() - renderNs;
    if (cold) {
      sCacheStats.cold++;
      sCacheStats.coldNs += renderNs;
    } else if (partial) {
      sCacheStats.partial++;
      sCacheStats.partialNs += renderNs;
    }
    sCacheStats.flush(mCache.valid, ringSize);
  }
  return true;
}

void SlintGraphicsView::refillRenderCache() noexcept {
  GraphicsScene* scene = mCacheScene.data();
  if ((!scene) || (!mRenderCacheEnabled) || mCache.valid.isEmpty()) {
    return;
  }
  if (mGlSurface && mGlContext &&
      ((!mGlError.isEmpty()) || (!mGlContext->makeCurrent(mGlSurface.get())))) {
    return;
  }
  QCoreApplication::sendPostedEvents(scene, QEvent::MetaCall);
  QList<QRectF> dirty;
  dirty = scene->takeDirtyRects();
  applyDirtyRects(*scene, dirty);
  if (mCache.valid.isEmpty()) {
    return;
  }

  QElapsedTimer timer;
  timer.start();
  const int margin = sCacheMargin;
  const QRect window = mCache.view.adjusted(-margin, -margin, margin, margin);
  const QRect v = mCache.valid;
  const int gaps[4] = {
      v.left() - window.left(),  // Left.
      window.right() - v.right(),  // Right.
      v.top() - window.top(),  // Top.
      window.bottom() - v.bottom(),  // Bottom.
  };
  const int side = static_cast<int>(std::max_element(gaps, gaps + 4) - gaps);
  const int thickness = std::min(gaps[side], sCacheQuantum);
  if (thickness <= 0) {
    return;
  }
  QRect chunk;
  switch (side) {
    case 0:
      chunk = QRect(v.left() - thickness, v.top(), thickness, v.height());
      break;
    case 1:
      chunk = QRect(v.right() + 1, v.top(), thickness, v.height());
      break;
    case 2:
      chunk = QRect(v.left(), v.top() - thickness, v.width(), thickness);
      break;
    default:
      chunk = QRect(v.left(), v.bottom() + 1, v.width(), thickness);
      break;
  }
  storeRegion(chunk, renderRegion(*scene, chunk));
  mCache.valid = v.united(chunk);

  if (sCacheLog) {
    sCacheStats.chunks++;
    sCacheStats.chunkNs += timer.nsecsElapsed();
  }
  if (mCache.valid != window) {
    mRefillTimer->start(1);
  }
}

QImage SlintGraphicsView::renderRegion(GraphicsScene& scene,
                                       const QRect& region) noexcept {
  const qreal scale = mCache.scale;
  const QSize size = region.size();
  const QRectF target(QPointF(0, 0), size);
  const QRectF source(mCache.mirror ? (-(region.x() + region.width()) / scale)
                                    : (region.x() / scale),
                      region.y() / scale, region.width() / scale,
                      region.height() / scale);
  auto draw = [&](QPaintDevice* device) {
    QPainter painter(device);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    if (mCache.mirror) {
      painter.translate(size.width(), 0);
      painter.scale(-1, 1);
    }
    scene.renderContent(&painter, target, source);
  };

  QElapsedTimer timer;
  timer.start();
  QImage img;
  if (mCache.openGl) {
    const QPair<int, int> key(size.width(), size.height());
    std::shared_ptr<QOpenGLFramebufferObject> fbo = mCacheFbos.value(key);
    if (!fbo) {
      if (mCacheFbos.count() >= 32) {
        mCacheFbos.clear();
      }
      QOpenGLFramebufferObjectFormat format;
      format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
      format.setSamples(4);
      fbo = std::make_shared<QOpenGLFramebufferObject>(size, format);
      mCacheFbos.insert(key, fbo);
    }
    if (fbo->bind()) {
      {
        QOpenGLPaintDevice device(size);
        draw(&device);
      }
      fbo->release();
      sCacheStats.drawNs += sCacheLog ? timer.nsecsElapsed() : 0;
      timer.restart();
      img = fbo->toImage();
      sCacheStats.readNs += sCacheLog ? timer.nsecsElapsed() : 0;
    }
  }
  if (img.isNull()) {
    img = QImage(size, QImage::Format_RGBA8888_Premultiplied);
    draw(&img);
    sCacheStats.drawNs += sCacheLog ? timer.nsecsElapsed() : 0;
  }
  return img;
}

void SlintGraphicsView::storeRegion(const QRect& region, QImage img) noexcept {
  QElapsedTimer timer;
  timer.start();
  if (img.format() != QImage::Format_RGBA8888_Premultiplied) {
    img.convertTo(QImage::Format_RGBA8888_Premultiplied);
  }
  const qint64 convNs = timer.nsecsElapsed();
  forEachRingSegment(region, mCache.ring.size(),
                     [&](const QRect& segment, const QPoint& pos) {
                       copyRows(img, segment, mCache.ring, pos);
                     });
  if (sCacheLog) {
    sCacheStats.regions++;
    sCacheStats.convNs += convNs;
    sCacheStats.storeNs += timer.nsecsElapsed() - convNs;
  }
}

void SlintGraphicsView::cropRegion(const QRect& region,
                                   QImage& out) const noexcept {
  forEachRingSegment(region, mCache.ring.size(),
                     [&](const QRect& segment, const QPoint& pos) {
                       copyRows(mCache.ring, QRect(pos, segment.size()), out,
                                segment.topLeft());
                     });
}

void SlintGraphicsView::applyDirtyRects(GraphicsScene& scene,
                                        const QList<QRectF>& dirty) noexcept {
  if (mCache.valid.isEmpty() || dirty.isEmpty()) {
    return;
  }

  QElapsedTimer timer;
  timer.start();
  const qreal scale = mCache.scale;
  const qreal limit = 1e8;
  const int pad = 2;
  const int align = 32;
  auto toDevice = [&](const QRectF& r) {
    const qreal x0 =
        qBound(-limit, (mCache.mirror ? -r.right() : r.left()) * scale, limit);
    const qreal x1 =
        qBound(-limit, (mCache.mirror ? -r.left() : r.right()) * scale, limit);
    const qreal y0 = qBound(-limit, r.top() * scale, limit);
    const qreal y1 = qBound(-limit, r.bottom() * scale, limit);
    const int ix0 = (qFloor((x0 - pad) / align)) * align;
    const int iy0 = (qFloor((y0 - pad) / align)) * align;
    const int ix1 = (qCeil((x1 + pad) / align)) * align;
    const int iy1 = (qCeil((y1 + pad) / align)) * align;
    return QRect(ix0, iy0, ix1 - ix0, iy1 - iy0).intersected(mCache.valid);
  };

  QRectF sceneBounds;
  for (const QRectF& r : dirty) {
    sceneBounds |= r;
  }
  const QRect bounds = toDevice(sceneBounds);
  if (bounds.isEmpty()) {
    return;
  }

  // Beyond this count the overhead of the separate regions alone exceeds the
  // cost of a full view render.
  const int maxSeparate =
      static_cast<int>(sFullViewRenderMs / sDirtyRegionOverheadMs);
  QList<QRect> rects;
  if ((dirty.count() > 1) && (dirty.count() <= maxSeparate)) {
    for (const QRectF& r : dirty) {
      const QRect rect = toDevice(r);
      if (!rect.isEmpty()) {
        rects.append(rect);
      }
    }
  } else {
    rects.append(bounds);
  }

  // Render either the separate regions or their bounding box, whichever is
  // estimated to be cheaper.
  const qint64 viewArea =
      static_cast<qint64>(mCache.view.width()) * mCache.view.height();
  auto cost = [viewArea](qint64 area) {
    return sDirtyRegionOverheadMs + (sFullViewRenderMs * area / viewArea);
  };
  qint64 area = static_cast<qint64>(bounds.width()) * bounds.height();
  if (rects.count() > 1) {
    // Collapse the longer list of rects into a region of unique rects
    QRegion region;
    for (const QRect& r : rects) {
      region += r;
    }

    // Sum the costs for the unique rects
    QList<QRect> separate;
    qint64 separateArea = 0;
    double separateCost = 0;
    for (const QRect& r : region) {
      const qint64 a = static_cast<qint64>(r.width()) * r.height();
      separate.append(r);
      separateArea += a;
      separateCost += cost(a);
    }

    // Select the cheaper option
    if (separateCost <= cost(area)) {
      rects = separate;
      area = separateArea;
    } else {
      rects = {bounds};
    }
  }
  if (area > (sDirtyAreaLimit * viewArea)) {
    mCache.valid = QRect();
    sCacheStats.dirtyFallbacks += sCacheLog ? 1 : 0;
    return;
  }

  for (const QRect& r : rects) {
    storeRegion(r, renderRegion(scene, r));
  }
  if (sCacheLog) {
    sCacheStats.dirtyUpdates++;
    sCacheStats.dirtyRects += rects.count();
    sCacheStats.dirtyNs += timer.nsecsElapsed();
  }
}

void SlintGraphicsView::invalidateRenderCache() noexcept {
  mRefillTimer->stop();
  mCache = RenderCache();
  mCacheScene.clear();
  mCacheFbos.clear();
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
