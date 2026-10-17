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
#include <gtest/gtest.h>
#include <librepcb/core/types/enums.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>
#include <librepcb/editor/graphics/graphicsscene.h>
#include <librepcb/editor/graphics/rendercache.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <algorithm>
#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class RenderCacheTest : public ::testing::Test {
protected:
  /**
   * Surface renderer which paints on a QImage and records its calls.
   */
  struct Surface {
    int calls = 0;
    QList<QSize> sizes;
    bool fail = false;

    RenderCache::SurfaceRenderer renderer() {
      return
          [this](const QSize& size, const RenderCache::PaintFunction& paint) {
            ++calls;
            sizes.append(size);
            if (fail) {
              return QImage();
            }
            QImage img(size, QImage::Format_RGBA8888_Premultiplied);
            img.fill(Qt::transparent);
            paint(&img);
            return img;
          };
    }
  };

  const QSize mViewSize = QSize(200, 150);
  Surface mSurface;
  int mRepaints = 0;
  GraphicsScene mScene;
  std::vector<std::unique_ptr<QGraphicsRectItem>> mItems;
  std::unique_ptr<RenderCache> mCache;

  RenderCacheTest() {
    // A pattern of rects wider and taller than any view used in the tests.
    for (int x = 0; x < 2400; x += 70) {
      for (int y = 0; y < 1200; y += 90) {
        addRect(QRectF(x + 5, y + 5, 40, 50),
                QColor::fromHsv((x * 7 + y * 3) % 360, 200, 220));
      }
    }
    mCache.reset(
        new RenderCache(mSurface.renderer(), [this]() { ++mRepaints; }));
    settleScene();
  }

  QGraphicsRectItem* addRect(const QRectF& rect, const QColor& color) {
    std::unique_ptr<QGraphicsRectItem> item(new QGraphicsRectItem(rect));
    item->setBrush(color);
    mScene.addItem(*item);
    mItems.push_back(std::move(item));
    return mItems.back().get();
  }

  // Simulates the event loop running between two frames.
  void settleScene() {
    for (int i = 0; i < 5; ++i) {
      QCoreApplication::processEvents();
    }
  }

  // Spins the event loop until the condition holds or a deadline expires.
  template <typename F>
  bool spinUntil(F&& condition, int timeoutMs = 3000) {
    QElapsedTimer timer;
    timer.start();
    while ((!condition()) && (timer.elapsed() < timeoutMs)) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
      QThread::msleep(1);
    }
    return condition();
  }

  // Spins the event loop until the cache stopped rendering for a while.
  void spinUntilIdle() {
    int last = -1;
    QElapsedTimer quiet;
    quiet.start();
    spinUntil([&]() {
      if (mSurface.calls != last) {
        last = mSurface.calls;
        quiet.restart();
      }
      return quiet.elapsed() > 100;
    });
  }

  // Reference rendering of a view without any caching.
  QImage renderUncached(const QPoint& pos, qreal scale, bool mirror) {
    QImage img(mViewSize, QImage::Format_RGBA8888_Premultiplied);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    if (mirror) {
      painter.translate(mViewSize.width(), 0);
      painter.scale(-1, 1);
    }
    mScene.renderContent(
        &painter, QRectF(QPointF(0, 0), mViewSize),
        QRectF(pos.x() / scale, pos.y() / scale, mViewSize.width() / scale,
               mViewSize.height() / scale));
    return img;
  }

  QImage renderCached(const QPoint& pos, qreal scale = 1, bool mirror = false,
                      bool* ok = nullptr) {
    QImage img(mViewSize, QImage::Format_RGBA8888_Premultiplied);
    img.fill(Qt::magenta);  // Make unwritten pixels obvious.
    const bool result =
        mCache->render(mScene, pos, mViewSize, scale, mirror, img);
    if (ok) {
      *ok = result;
    }
    return img;
  }

  // Antialiased edges can differ by a rounding step in the coverage when the
  // same content is rendered in differently shaped regions, hence a small
  // tolerance. Missing or stale content differs by far more.
  static ::testing::AssertionResult sameImages(const QImage& actual,
                                               const QImage& expected) {
    static const int tolerance = 2;
    if (actual.size() != expected.size()) {
      return ::testing::AssertionFailure() << "different image sizes";
    }
    for (int y = 0; y < actual.height(); ++y) {
      for (int x = 0; x < actual.width(); ++x) {
        const QRgb a = actual.pixel(x, y);
        const QRgb e = expected.pixel(x, y);
        const int diff = std::max(
            std::max(qAbs(qRed(a) - qRed(e)), qAbs(qGreen(a) - qGreen(e))),
            std::max(qAbs(qBlue(a) - qBlue(e)), qAbs(qAlpha(a) - qAlpha(e))));
        if (diff > tolerance) {
          return ::testing::AssertionFailure()
              << "first difference at " << x << "," << y << ": " << std::hex
              << a << " != " << e;
        }
      }
    }
    return ::testing::AssertionSuccess();
  }
};

/*******************************************************************************
 *  Test Methods: Static Helpers
 ******************************************************************************/

TEST(RenderCacheHelpersTest, testWrapIndex) {
  EXPECT_EQ(0, RenderCache::wrapIndex(0, 5));
  EXPECT_EQ(4, RenderCache::wrapIndex(4, 5));
  EXPECT_EQ(0, RenderCache::wrapIndex(5, 5));
  EXPECT_EQ(2, RenderCache::wrapIndex(12, 5));
  EXPECT_EQ(4, RenderCache::wrapIndex(-1, 5));
  EXPECT_EQ(0, RenderCache::wrapIndex(-5, 5));
  EXPECT_EQ(4, RenderCache::wrapIndex(-6, 5));
  EXPECT_EQ(3, RenderCache::wrapIndex(-12, 5));
}

TEST(RenderCacheHelpersTest, testSnapPos) {
  QPoint snapped;
  EXPECT_TRUE(RenderCache::snapPos(QPointF(10.4, -10.6), snapped));
  EXPECT_EQ(QPoint(10, -11), snapped);
  EXPECT_TRUE(RenderCache::snapPos(QPointF(-5e8, 5e8), snapped));
  EXPECT_EQ(QPoint(-500000000, 500000000), snapped);
  EXPECT_FALSE(RenderCache::snapPos(QPointF(2e9, 0), snapped));
  EXPECT_FALSE(RenderCache::snapPos(QPointF(0, -2e9), snapped));
  EXPECT_FALSE(RenderCache::snapPos(QPointF(qQNaN(), 0), snapped));
}

TEST(RenderCacheHelpersTest, testSubtractRect) {
  // Checks that the pieces cover exactly outer minus inner, without overlap.
  auto check = [](const QRect& outer, const QRect& inner) {
    const QList<QRect> pieces = RenderCache::subtractRect(outer, inner);
    qint64 area = 0;
    for (int i = 0; i < pieces.count(); ++i) {
      EXPECT_FALSE(pieces[i].isEmpty());
      EXPECT_TRUE(outer.contains(pieces[i]));
      EXPECT_FALSE(pieces[i].intersects(inner));
      for (int k = i + 1; k < pieces.count(); ++k) {
        EXPECT_FALSE(pieces[i].intersects(pieces[k]));
      }
      area += static_cast<qint64>(pieces[i].width()) * pieces[i].height();
    }
    const QRect common = outer.intersected(inner);
    const qint64 commonArea = common.isEmpty()
        ? 0
        : (static_cast<qint64>(common.width()) * common.height());
    EXPECT_EQ(static_cast<qint64>(outer.width()) * outer.height() - commonArea,
              area);
    return pieces;
  };

  const QRect outer(0, 0, 100, 100);
  EXPECT_EQ(QList<QRect>{outer}, check(outer, QRect(200, 200, 10, 10)));
  EXPECT_EQ(QList<QRect>{outer}, check(outer, QRect(100, 0, 50, 100)));
  EXPECT_TRUE(check(outer, QRect(-10, -10, 200, 200)).isEmpty());
  EXPECT_TRUE(check(outer, outer).isEmpty());
  EXPECT_EQ(4, check(outer, QRect(30, 30, 40, 40)).count());
  EXPECT_EQ(2, check(outer, QRect(50, 50, 100, 100)).count());
  EXPECT_EQ(1, check(outer, QRect(-20, -20, 70, 120)).count());
  EXPECT_EQ(1, check(outer, QRect(0, 0, 100, 40)).count());
  EXPECT_EQ(2, check(outer, QRect(0, 30, 100, 40)).count());
}

TEST(RenderCacheHelpersTest, testMergeValid) {
  // Same rows: extend horizontally.
  EXPECT_EQ(
      QRect(0, 0, 150, 50),
      RenderCache::mergeValid(QRect(0, 0, 100, 50), QRect(50, 0, 100, 50)));
  // Same columns: extend vertically.
  EXPECT_EQ(
      QRect(0, 0, 50, 150),
      RenderCache::mergeValid(QRect(0, 0, 50, 100), QRect(0, 50, 50, 100)));
  // View covers more than the old rect in both directions: view only.
  EXPECT_EQ(
      QRect(50, 50, 100, 100),
      RenderCache::mergeValid(QRect(0, 0, 100, 100), QRect(50, 50, 100, 100)));
  // The result always contains the view.
  const QRect old(0, 0, 100, 100);
  const QRect view(20, 20, 30, 30);
  const QRect merged = RenderCache::mergeValid(old, view);
  EXPECT_TRUE(merged.contains(view));
  EXPECT_TRUE(old.contains(merged));
}

/*******************************************************************************
 *  Test Methods: Cache Behavior
 ******************************************************************************/

TEST_F(RenderCacheTest, testFirstRenderRendersViewOnce) {
  bool ok = false;
  const QImage img = renderCached(QPoint(0, 0), 1, false, &ok);
  EXPECT_TRUE(ok);
  EXPECT_EQ(1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.first());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));

  // Identical frame comes straight from the cache.
  const QImage again = renderCached(QPoint(0, 0));
  EXPECT_EQ(1, mSurface.calls);
  EXPECT_TRUE(sameImages(again, img));
}

TEST_F(RenderCacheTest, testPanRendersOnlyExposedStrip) {
  renderCached(QPoint(0, 0));
  ASSERT_EQ(1, mSurface.calls);

  const QImage img = renderCached(QPoint(20, 0));
  ASSERT_EQ(2, mSurface.calls);
  EXPECT_EQ(QSize(20, mViewSize.height()), mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(20, 0), 1, false)));
}

TEST_F(RenderCacheTest, testPanBeyondRingSizeMatchesUncachedRendering) {
  // The total distance exceeds the ring size in both directions, so the
  // cached pixels wrap around the ring several times.
  QPoint pos(0, 0);
  for (int i = 0; i < 40; ++i) {
    pos += QPoint(i % 3 == 2 ? -37 : 61, i % 2 ? 29 : -11);
    const QImage img = renderCached(pos);
    ASSERT_TRUE(sameImages(img, renderUncached(pos, 1, false)))
        << "step " << i << " at " << pos.x() << "," << pos.y();
  }
}

TEST_F(RenderCacheTest, testGridDoesNotDependOnRenderedRegions) {
  mScene.setBackgroundColors(Qt::black, Qt::white);
  for (GridStyle style : {GridStyle::Dots, GridStyle::Lines}) {
    for (int interval : {800000, 910000}) {
      SCOPED_TRACE(testing::Message() << "style " << static_cast<int>(style)
                                      << ", interval " << interval);
      mScene.setGridStyle(style);
      mScene.setGridInterval(PositiveLength(interval));
      settleScene();
      mCache->invalidate();

      // Strips of various widths in all directions, each one rendered on its
      // own, must join seamlessly.
      QPoint pos(0, 0);
      for (int i = 0; i < 30; ++i) {
        pos += QPoint((i % 4 == 3) ? -13 : 7 + (i % 5) * 3,
                      (i % 3 == 2) ? -11 : 5 + (i % 4) * 2);
        const QImage img = renderCached(pos);
        ASSERT_TRUE(sameImages(img, renderUncached(pos, 1, false)))
            << "step " << i << " at " << pos.x() << "," << pos.y();
      }
    }
  }
}

TEST_F(RenderCacheTest, testMirroredViewMatchesUncachedRendering) {
  QPoint pos(30, 10);
  for (int i = 0; i < 10; ++i) {
    pos += QPoint(45, 17);
    const QImage img = renderCached(pos, 1, true);
    ASSERT_TRUE(sameImages(img, renderUncached(pos, 1, true))) << "step " << i;
  }
}

TEST_F(RenderCacheTest, testIdleRefillCompletesMargin) {
  renderCached(QPoint(100, 100));
  spinUntilIdle();
  EXPECT_GT(mSurface.calls, 1);

  // Every move stays within the margin of the previous view, so it must be
  // served without rendering once the margin has been refilled.
  for (const QPoint& pos : {QPoint(0, 0), QPoint(200, 100), QPoint(100, 200),
                            QPoint(0, 100), QPoint(100, 100)}) {
    const int calls = mSurface.calls;
    const QImage img = renderCached(pos);
    EXPECT_EQ(calls, mSurface.calls) << pos.x() << "," << pos.y();
    EXPECT_TRUE(sameImages(img, renderUncached(pos, 1, false)));
    spinUntilIdle();
  }
}

TEST_F(RenderCacheTest, testChangedItemRerendersOnlyItsRegion) {
  renderCached(QPoint(0, 0));
  const int calls = mSurface.calls;

  mItems.front()->setBrush(Qt::red);
  settleScene();
  const QImage img = renderCached(QPoint(0, 0));
  EXPECT_EQ(calls + 1, mSurface.calls);
  const QSize size = mSurface.sizes.last();
  EXPECT_GE(size.width(), 40);
  EXPECT_GE(size.height(), 50);
  EXPECT_LT(size.width(), mViewSize.width());
  EXPECT_LT(size.height(), mViewSize.height());
  EXPECT_EQ(0, size.width() % 32);
  EXPECT_EQ(0, size.height() % 32);
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));
}

TEST_F(RenderCacheTest, testChangeCoveringMostOfViewRerendersView) {
  renderCached(QPoint(0, 0));
  const int calls = mSurface.calls;

  addRect(QRectF(-100, -100, 600, 600), Qt::green);
  settleScene();
  const QImage img = renderCached(QPoint(0, 0));
  EXPECT_EQ(calls + 1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));
}

TEST_F(RenderCacheTest, testForegroundOnlyChangeDoesNotRender) {
  renderCached(QPoint(0, 0));
  const int calls = mSurface.calls;

  mScene.setSelectionRect(Point::fromPx(QPointF(10, 10)),
                          Point::fromPx(QPointF(80, 60)));
  settleScene();
  renderCached(QPoint(0, 0));
  mScene.clearSelectionRect();
  settleScene();
  renderCached(QPoint(0, 0));
  EXPECT_EQ(calls, mSurface.calls);
}

TEST_F(RenderCacheTest, testBackgroundChangeRerendersEverything) {
  renderCached(QPoint(0, 0));
  const int calls = mSurface.calls;

  mScene.setBackgroundColors(Qt::yellow, Qt::black);
  const QImage img = renderCached(QPoint(0, 0));
  EXPECT_EQ(calls + 1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));
}

TEST_F(RenderCacheTest, testZoomShowsPreviewUntilSettled) {
  renderCached(QPoint(0, 0));
  spinUntilIdle();
  const int calls = mSurface.calls;
  mRepaints = 0;

  // The preview is drawn from the cached pixels without rendering.
  bool ok = false;
  const QImage preview = renderCached(QPoint(0, 0), 2, false, &ok);
  EXPECT_TRUE(ok);
  EXPECT_EQ(calls, mSurface.calls);
  EXPECT_EQ(0, mRepaints);

  // Once the zoom has settled, the owner is asked to repaint.
  ASSERT_TRUE(spinUntil([this]() { return mRepaints > 0; }));
  EXPECT_EQ(calls, mSurface.calls);

  // The following frame renders the view at the new scale.
  const QImage img = renderCached(QPoint(0, 0), 2);
  EXPECT_EQ(calls + 1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 2, false)));
}

TEST_F(RenderCacheTest, testScaleChangeWithoutCacheRendersImmediately) {
  renderCached(QPoint(0, 0), 2);
  EXPECT_EQ(1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.first());
  EXPECT_EQ(0, mRepaints);
}

TEST_F(RenderCacheTest, testInvalidateDropsCache) {
  renderCached(QPoint(0, 0));
  spinUntilIdle();
  const int calls = mSurface.calls;

  mCache->invalidate();
  spinUntilIdle();
  EXPECT_EQ(calls, mSurface.calls);  // Refill is cancelled.
  const QImage img = renderCached(QPoint(0, 0));
  EXPECT_EQ(calls + 1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));
}

TEST_F(RenderCacheTest, testRendererFailureIsReportedAndNotCached) {
  mSurface.fail = true;
  bool ok = true;
  renderCached(QPoint(0, 0), 1, false, &ok);
  EXPECT_FALSE(ok);

  // Nothing is marked valid, so the next frame renders the whole view.
  mSurface.fail = false;
  const int calls = mSurface.calls;
  const QImage img = renderCached(QPoint(0, 0), 1, false, &ok);
  EXPECT_TRUE(ok);
  EXPECT_EQ(calls + 1, mSurface.calls);
  EXPECT_EQ(mViewSize, mSurface.sizes.last());
  EXPECT_TRUE(sameImages(img, renderUncached(QPoint(0, 0), 1, false)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
