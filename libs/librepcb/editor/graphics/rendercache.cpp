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

// AI DISCLAIMER: Claude AI assisted in the creation of this file.

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "rendercache.h"

#include "graphicsscene.h"

#include <algorithm>
#include <cstring>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static constexpr int sCacheMargin = 256;
static constexpr int sCacheQuantum = 64;
static constexpr int sRefillIntervalMs = 1;

// Estimated cost model to choose between rendering separate dirty regions or
// their bounding box.
static constexpr double sDirtyRegionOverheadMs = 1.5;
static constexpr double sFullViewRenderMs = 50.0;

// Fraction of the view above which dirty regions are not worth rendering
// separately from the full view.
static constexpr double sDirtyAreaLimit = 0.8;

// Idle time after the last zoom step before the scene is rendered at the new
// scale.
static constexpr int sZoomSettleMs = 50;

// Splits a rect of the wrapped device space into contiguous ring segments and
// calls f(segment relative to rect, ring position) for each of them.
template <typename F>
static void forEachRingSegment(const QRect& rect, const QSize& ring, F&& f) {
  for (int y = 0; y < rect.height();) {
    const int ry = RenderCache::wrapIndex(rect.y() + y, ring.height());
    const int h = std::min(rect.height() - y, ring.height() - ry);
    for (int x = 0; x < rect.width();) {
      const int rx = RenderCache::wrapIndex(rect.x() + x, ring.width());
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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RenderCache::RenderCache(SurfaceRenderer renderer,
                         RepaintRequest repaint) noexcept
  : mRenderer(renderer),
    mRepaint(repaint),
    mScale(0),
    mMirror(false),
    mZoomSettled(false) {
  mRefillTimer.setSingleShot(true);
  QObject::connect(&mRefillTimer, &QTimer::timeout, [this]() { refill(); });
  mZoomSettleTimer.setSingleShot(true);
  QObject::connect(&mZoomSettleTimer, &QTimer::timeout,
                   [this]() { settle(); });
}

RenderCache::~RenderCache() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool RenderCache::render(GraphicsScene& scene, const QPoint& pos,
                         const QSize& size, qreal scale, bool mirror,
                         QImage& out) noexcept {
  // Deliver pending scene change notifications to get the current changes.
  QCoreApplication::sendPostedEvents(&scene, QEvent::MetaCall);
  QList<QRectF> dirty;
  dirty = scene.takeDirtyRects();

  const QRect view(
      QPoint(mirror ? (-pos.x() - size.width()) : pos.x(), pos.y()), size);
  const int margin = sCacheMargin;
  const QSize ringSize(size.width() + 2 * margin, size.height() + 2 * margin);
  const QRect window = view.adjusted(-margin, -margin, margin, margin);

  const bool sameLayout = (mRing.size() == ringSize) && (mMirror == mirror) &&
      (mScene == &scene);

  // While zooming, the cached pixels are shown scaled instead of rendering the
  // scene at every step.
  QRectF previewSrc;
  if (sameLayout && (mScale != scale) && (!mZoomSettled)) {
    const qreal ratio = mScale / scale;
    previewSrc = QRectF(view.x() * ratio, view.y() * ratio,
                        view.width() * ratio, view.height() * ratio)
                     .intersected(QRectF(mValid));
  }
  const bool preview = (previewSrc.width() >= 1) && (previewSrc.height() >= 1);

  if (preview) {
    mRefillTimer.stop();
    mZoomSettleTimer.start(sZoomSettleMs);
    drawZoomPreview(scene, view, scale, previewSrc, out);
    return true;
  }

  mZoomSettled = false;
  mZoomSettleTimer.stop();
  const bool match = sameLayout && (mScale == scale);
  if (!match) {
    if (mRing.size() != ringSize) {
      mRing = QImage(ringSize, QImage::Format_RGBA8888_Premultiplied);
    }
    mValid = QRect();
    mScale = scale;
    mMirror = mirror;
    mScene = &scene;
  }

  mValid = mValid.intersected(window);
  mView = view;
  applyDirtyRects(scene, dirty);
  if (!mValid.contains(view)) {
    if (!mValid.intersects(view)) {
      if (!renderAndStore(scene, view)) {
        mValid = QRect();
        return false;
      }
      mValid = view;
    } else {
      const QRect old = mValid;
      for (const QRect& piece : subtractRect(view, old)) {
        if (!renderAndStore(scene, piece)) {
          mValid = QRect();
          return false;
        }
      }
      mValid = mergeValid(old, view);
    }
  }
  cropRegion(view, out);

  if ((mValid != window) && (!mRefillTimer.isActive())) {
    mRefillTimer.start(sRefillIntervalMs);
  }
  return true;
}

void RenderCache::invalidate() noexcept {
  mRefillTimer.stop();
  mZoomSettleTimer.stop();
  mZoomSettled = false;
  mRing = QImage();
  mValid = QRect();
  mView = QRect();
  mScale = 0;
  mMirror = false;
  mScene.clear();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

int RenderCache::wrapIndex(int value, int size) noexcept {
  const int m = value % size;
  return (m < 0) ? (m + size) : m;
}

QList<QRect> RenderCache::subtractRect(const QRect& outer,
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

QRect RenderCache::mergeValid(const QRect& old, const QRect& view) noexcept {
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool RenderCache::renderAndStore(GraphicsScene& scene,
                                 const QRect& region) noexcept {
  QImage img = renderRegion(scene, region);
  if (img.isNull()) {
    return false;
  }
  storeRegion(region, img);
  return true;
}

QImage RenderCache::renderRegion(GraphicsScene& scene,
                                 const QRect& region) noexcept {
  const qreal scale = mScale;
  const QSize size = region.size();
  const QRectF target(QPointF(0, 0), size);
  const QRectF source(mMirror ? (-(region.x() + region.width()) / scale)
                              : (region.x() / scale),
                      region.y() / scale, region.width() / scale,
                      region.height() / scale);
  auto draw = [&](QPaintDevice* device) {
    QPainter painter(device);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    if (mMirror) {
      painter.translate(size.width(), 0);
      painter.scale(-1, 1);
    }
    scene.renderContent(&painter, target, source);
  };
  return mRenderer(size, draw);
}

void RenderCache::storeRegion(const QRect& region, QImage img) noexcept {
  if (img.format() != QImage::Format_RGBA8888_Premultiplied) {
    img.convertTo(QImage::Format_RGBA8888_Premultiplied);
  }
  forEachRingSegment(region, mRing.size(),
                     [&](const QRect& segment, const QPoint& pos) {
                       copyRows(img, segment, mRing, pos);
                     });
}

void RenderCache::cropRegion(const QRect& region, QImage& out) const noexcept {
  forEachRingSegment(region, mRing.size(),
                     [&](const QRect& segment, const QPoint& pos) {
                       copyRows(mRing, QRect(pos, segment.size()), out,
                                segment.topLeft());
                     });
}

void RenderCache::applyDirtyRects(GraphicsScene& scene,
                                  const QList<QRectF>& dirty) noexcept {
  if (mValid.isEmpty() || dirty.isEmpty()) {
    return;
  }

  const qreal scale = mScale;
  const qreal limit = 1e8;
  const int pad = 2;
  const int align = 32;
  auto toDevice = [&](const QRectF& r) {
    const qreal x0 =
        qBound(-limit, (mMirror ? -r.right() : r.left()) * scale, limit);
    const qreal x1 =
        qBound(-limit, (mMirror ? -r.left() : r.right()) * scale, limit);
    const qreal y0 = qBound(-limit, r.top() * scale, limit);
    const qreal y1 = qBound(-limit, r.bottom() * scale, limit);
    const int ix0 = (qFloor((x0 - pad) / align)) * align;
    const int iy0 = (qFloor((y0 - pad) / align)) * align;
    const int ix1 = (qCeil((x1 + pad) / align)) * align;
    const int iy1 = (qCeil((y1 + pad) / align)) * align;
    return QRect(ix0, iy0, ix1 - ix0, iy1 - iy0).intersected(mValid);
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
  const qint64 viewArea = static_cast<qint64>(mView.width()) * mView.height();
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
    mValid = QRect();
    return;
  }

  for (const QRect& r : rects) {
    if (!renderAndStore(scene, r)) {
      mValid = QRect();
      return;
    }
  }
}

void RenderCache::drawZoomPreview(const GraphicsScene& scene,
                                  const QRect& view, qreal scale,
                                  const QRectF& src,
                                  QImage& out) const noexcept {
  const qreal ratio = mScale / scale;
  const QRect srcPx = src.toAlignedRect();
  QImage tmp(srcPx.size(), QImage::Format_RGBA8888_Premultiplied);
  cropRegion(srcPx, tmp);

  out.fill(scene.getBackgroundColor());
  QPainter painter(&out);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  const QRectF target(src.x() / ratio - view.x(), src.y() / ratio - view.y(),
                      src.width() / ratio, src.height() / ratio);
  painter.drawImage(target, tmp, src.translated(-srcPx.topLeft()));
}

void RenderCache::refill() noexcept {
  GraphicsScene* scene = mScene.data();
  if ((!scene) || mValid.isEmpty()) {
    return;
  }
  QCoreApplication::sendPostedEvents(scene, QEvent::MetaCall);
  QList<QRectF> dirty;
  dirty = scene->takeDirtyRects();
  applyDirtyRects(*scene, dirty);
  if (mValid.isEmpty()) {
    return;
  }

  const int margin = sCacheMargin;
  const QRect window = mView.adjusted(-margin, -margin, margin, margin);
  const QRect v = mValid;
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
  if (!renderAndStore(*scene, chunk)) {
    return;
  }
  mValid = v.united(chunk);

  if (mValid != window) {
    mRefillTimer.start(sRefillIntervalMs);
  }
}

void RenderCache::settle() noexcept {
  mZoomSettled = true;
  if (mRepaint) {
    mRepaint();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
