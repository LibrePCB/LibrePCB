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

// AI DISCLAIMER: Claude AI assisted in the creation/modification of this file.

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/editor/graphics/graphicsscene.h>

#include <QtTest>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GraphicsSceneTest : public ::testing::Test {
protected:
  const QSize mSize = QSize(200, 150);
  const QPointF mOffset = QPointF(-30, -20);
  const qreal mScale = 2;
  GraphicsScene mScene;
  std::vector<std::unique_ptr<QGraphicsRectItem>> mItems;

  GraphicsSceneTest() {
    addRect(QRectF(-20, -10, 60, 40), Qt::red);
    addRect(QRectF(10, 20, 50, 30), Qt::blue);
    addRect(QRectF(50, -5, 30, 60), Qt::green);
    showForeground(true);
  }

  void addRect(const QRectF& rect, const QColor& color) {
    mItems.push_back(std::make_unique<QGraphicsRectItem>(rect));
    mItems.back()->setBrush(color);
    mItems.back()->setPen(QPen(Qt::black, 0));
    mScene.addItem(*mItems.back());
  }

  void showForeground(bool show) {
    GraphicsSceneCursor cursor;
    cursor.cross = show;
    mScene.setOriginCrossVisible(show);
    mScene.setSceneCursor(Point::fromPx(QPointF(40, 30)), cursor);
  }

  QTransform sceneToDevice(bool mirror) const {
    QTransform tf;
    if (mirror) {
      tf.translate(mSize.width(), 0);
      tf.scale(-mScale, mScale);
    } else {
      tf.scale(mScale, mScale);
    }
    tf.translate(-mOffset.x(), -mOffset.y());
    return tf;
  }

  QImage newImage() const {
    QImage img(mSize, QImage::Format_RGBA8888_Premultiplied);
    img.fill(Qt::transparent);
    return img;
  }

  /// Render the whole scene in one go, the way it was done before the
  /// foreground layer could be rendered separately.
  QImage renderAtOnce(bool mirror) {
    QImage img = newImage();
    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    QRectF source(0, 0, mSize.width() / mScale, mSize.height() / mScale);
    source.translate(mOffset);
    if (mirror) {
      painter.translate(mSize.width(), 0);
      painter.scale(-1, 1);
    }
    mScene.render(&painter, QRectF(QPointF(0, 0), mSize), source);
    return img;
  }

  QImage renderContent(bool mirror) {
    QImage img = newImage();
    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    mScene.renderContent(&painter, mSize, sceneToDevice(mirror));
    return img;
  }

  QImage renderForeground(bool mirror) {
    QImage img = newImage();
    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform);
    mScene.renderForeground(&painter, mSize, sceneToDevice(mirror));
    return img;
  }

  /// Largest difference of any channel of any pixel, or -1 if the sizes
  /// differ.
  static int maxDifference(const QImage& a, const QImage& b) {
    if (a.size() != b.size()) {
      return -1;
    }
    int result = 0;
    for (int y = 0; y < a.height(); ++y) {
      const uchar* pa = a.constScanLine(y);
      const uchar* pb = b.constScanLine(y);
      for (int i = 0; i < a.width() * 4; ++i) {
        result = std::max(result, std::abs(int(pa[i]) - int(pb[i])));
      }
    }
    return result;
  }

  static bool isTransparent(const QImage& img) {
    for (int y = 0; y < img.height(); ++y) {
      const uchar* p = img.constScanLine(y);
      if (std::any_of(p, p + img.width() * 4, [](uchar v) { return v != 0; })) {
        return false;
      }
    }
    return true;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GraphicsSceneTest, testContentAndForegroundMatchRenderAtOnce) {
  for (bool mirror : {false, true}) {
    QImage img = renderContent(mirror);
    {
      QPainter painter(&img);
      painter.setRenderHints(QPainter::Antialiasing |
                             QPainter::SmoothPixmapTransform);
      mScene.renderForeground(&painter, mSize, sceneToDevice(mirror));
    }
    const int diff = maxDifference(img, renderAtOnce(mirror));
    EXPECT_GE(diff, 0) << "mirror: " << mirror;
    EXPECT_LE(diff, 2) << "mirror: " << mirror;
  }
}

TEST_F(GraphicsSceneTest, testContentExcludesForeground) {
  for (bool mirror : {false, true}) {
    const QImage withForeground = renderContent(mirror);
    showForeground(false);
    const QImage withoutForeground = renderContent(mirror);
    showForeground(true);
    EXPECT_EQ(0, maxDifference(withForeground, withoutForeground))
        << "mirror: " << mirror;
  }
}

TEST_F(GraphicsSceneTest, testForegroundExcludesContent) {
  for (bool mirror : {false, true}) {
    EXPECT_FALSE(isTransparent(renderForeground(mirror)))
        << "mirror: " << mirror;
    showForeground(false);
    EXPECT_TRUE(isTransparent(renderForeground(mirror)))
        << "mirror: " << mirror;
    showForeground(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
