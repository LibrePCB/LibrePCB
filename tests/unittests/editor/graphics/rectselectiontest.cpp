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
#include <librepcb/editor/graphics/graphicsscene.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Helper Types
 ******************************************************************************/

// Minimal QGraphicsItem stand-in with a getVisibleShape() distinct from its
// shape(), so #footprintOf()/#hitsVisible() can be verified to use the
// former rather than falling back to the latter.
class RectSelectionTestItem : public QGraphicsRectItem {
public:
  explicit RectSelectionTestItem(const QRectF& clickRect,
                                 const QRectF& visibleRect) noexcept
    : QGraphicsRectItem(clickRect), mVisibleRect(visibleRect) {}

  QPainterPath getVisibleShape() const noexcept {
    QPainterPath p;
    p.addRect(mVisibleRect);
    return p;
  }

private:
  QRectF mVisibleRect;
};

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class RectSelectionTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods: hits(const QPainterPath&)
 ******************************************************************************/

TEST(RectSelectionTest, testHitsPathEmptyNeverHits) {
  const QPainterPath empty;
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Crossing)
          .hits(empty));
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Window)
          .hits(empty));
}

TEST(RectSelectionTest, testHitsPathCrossingOnOverlap) {
  QPainterPath path;
  path.addRect(QRectF(0, 0, 10, 10));
  const RectSelection selection(QRectF(5, 5, 10, 10),
                                RectSelection::Mode::Crossing);
  EXPECT_TRUE(selection.hits(path));
}

TEST(RectSelectionTest, testHitsPathCrossingNotTouching) {
  QPainterPath path;
  path.addRect(QRectF(0, 0, 10, 10));
  const RectSelection selection(QRectF(100, 100, 10, 10),
                                RectSelection::Mode::Crossing);
  EXPECT_FALSE(selection.hits(path));
}

TEST(RectSelectionTest, testHitsPathWindowEnclosed) {
  QPainterPath path;
  path.addRect(QRectF(1, 1, 8, 8));
  const RectSelection selection(QRectF(0, 0, 10, 10),
                                RectSelection::Mode::Window);
  EXPECT_TRUE(selection.hits(path));
}

TEST(RectSelectionTest, testHitsPathWindowPartialOverlap) {
  QPainterPath path;
  path.addRect(QRectF(5, 5, 10, 10));
  const RectSelection selection(QRectF(0, 0, 10, 10),
                                RectSelection::Mode::Window);
  EXPECT_FALSE(selection.hits(path));
}

TEST(RectSelectionTest, testHitsPathWindowNotTouching) {
  QPainterPath path;
  path.addRect(QRectF(100, 100, 10, 10));
  const RectSelection selection(QRectF(0, 0, 10, 10),
                                RectSelection::Mode::Window);
  EXPECT_FALSE(selection.hits(path));
}

/*******************************************************************************
 *  Test Methods: hits(const QGraphicsItem&)
 ******************************************************************************/

TEST(RectSelectionTest, testHitsItemUsesShape) {
  QGraphicsRectItem item(QRectF(0, 0, 10, 10));
  // QGraphicsRectItem's default pen (width 0, cosmetic) is treated as a
  // half-pixel-wide stroke for shape() purposes, which would make the shape
  // ~0.5px larger on each side than the rect below and defeat the exact
  // window-containment check further down. Use NoPen so shape() is exactly
  // the bare rect this test is actually about.
  item.setPen(Qt::NoPen);
  item.setPos(100, 100);  // Shape in scene coords is now (100,100,10,10).

  const RectSelection crossingAtOrigin(QRectF(0, 0, 10, 10),
                                       RectSelection::Mode::Crossing);
  EXPECT_FALSE(crossingAtOrigin.hits(item));

  const RectSelection crossingAtItem(QRectF(105, 105, 10, 10),
                                     RectSelection::Mode::Crossing);
  EXPECT_TRUE(crossingAtItem.hits(item));

  const RectSelection windowAroundItem(QRectF(100, 100, 10, 10),
                                       RectSelection::Mode::Window);
  EXPECT_TRUE(windowAroundItem.hits(item));
}

/*******************************************************************************
 *  Test Methods: hits(const QVector<QPainterPath>&)
 ******************************************************************************/

TEST(RectSelectionTest, testHitsCompositeEmptyVector) {
  const QVector<QPainterPath> parts;
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Crossing)
          .hits(parts));
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Window)
          .hits(parts));
}

TEST(RectSelectionTest, testHitsCompositeAllPartsEmpty) {
  QVector<QPainterPath> parts{QPainterPath(), QPainterPath()};
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Crossing)
          .hits(parts));
  EXPECT_FALSE(
      RectSelection(QRectF(-100, -100, 200, 200), RectSelection::Mode::Window)
          .hits(parts));
}

TEST(RectSelectionTest, testHitsCompositeCrossingAnyPart) {
  QPainterPath farAway;
  farAway.addRect(QRectF(1000, 1000, 10, 10));
  QPainterPath touched;
  touched.addRect(QRectF(0, 0, 10, 10));
  const QVector<QPainterPath> parts{farAway, touched};

  const RectSelection selection(QRectF(5, 5, 10, 10),
                                RectSelection::Mode::Crossing);
  EXPECT_TRUE(selection.hits(parts));
}

TEST(RectSelectionTest, testHitsCompositeCrossingNoPart) {
  QPainterPath a;
  a.addRect(QRectF(1000, 1000, 10, 10));
  QPainterPath b;
  b.addRect(QRectF(2000, 2000, 10, 10));
  const QVector<QPainterPath> parts{a, b};

  const RectSelection selection(QRectF(5, 5, 10, 10),
                                RectSelection::Mode::Crossing);
  EXPECT_FALSE(selection.hits(parts));
}

TEST(RectSelectionTest, testHitsCompositeWindowAllParts) {
  QPainterPath a;
  a.addRect(QRectF(1, 1, 2, 2));
  QPainterPath b;
  b.addRect(QRectF(5, 5, 2, 2));
  const QVector<QPainterPath> allEnclosed{a, b};

  const RectSelection selection(QRectF(0, 0, 10, 10),
                                RectSelection::Mode::Window);
  EXPECT_TRUE(selection.hits(allEnclosed));

  QPainterPath outside;
  outside.addRect(QRectF(100, 100, 2, 2));
  const QVector<QPainterPath> oneOutside{a, outside};
  EXPECT_FALSE(selection.hits(oneOutside));
}

TEST(RectSelectionTest, testHitsCompositeWindowIgnoresEmpty) {
  // An empty part (e.g. an item on a hidden layer) must not by itself make
  // the composite footprint "not enclosed" in window mode.
  QPainterPath enclosed;
  enclosed.addRect(QRectF(1, 1, 2, 2));
  const QVector<QPainterPath> parts{enclosed, QPainterPath()};

  const RectSelection selection(QRectF(0, 0, 10, 10),
                                RectSelection::Mode::Window);
  EXPECT_TRUE(selection.hits(parts));
}

/*******************************************************************************
 *  Test Methods: footprintOf() / hitsVisible()
 ******************************************************************************/

TEST(RectSelectionTest, testFootprintOfUsesVisibleShape) {
  // Click shape is large; visible shape is a small rect fully inside it.
  RectSelectionTestItem item(QRectF(0, 0, 100, 100), QRectF(40, 40, 5, 5));
  item.setPos(0, 0);

  const QPainterPath footprint = RectSelection::footprintOf(item);
  EXPECT_EQ(QRectF(40, 40, 5, 5), footprint.boundingRect());
}

TEST(RectSelectionTest, testHitsVisibleUsesVisibleShape) {
  // Click shape covers the selection rect, but the (much smaller) visible
  // shape does not: hitsVisible() must not select in crossing mode either.
  RectSelectionTestItem item(QRectF(0, 0, 100, 100), QRectF(90, 90, 5, 5));
  item.setPos(0, 0);

  const RectSelection crossingNearOrigin(QRectF(0, 0, 10, 10),
                                         RectSelection::Mode::Crossing);
  EXPECT_FALSE(crossingNearOrigin.hitsVisible(item));

  const RectSelection crossingOverVisible(QRectF(85, 85, 15, 15),
                                          RectSelection::Mode::Crossing);
  EXPECT_TRUE(crossingOverVisible.hitsVisible(item));
}

/*******************************************************************************
 *  Test Methods: GraphicsSceneMouseEvent::getRectSelectionMode()
 ******************************************************************************/

TEST(RectSelectionTest, testGetRectSelectionModeDragRight) {
  GraphicsSceneMouseEvent e;
  e.downScreenPos = QPointF(0, 0);
  e.screenPos = QPointF(10, 0);
  EXPECT_EQ(RectSelection::Mode::Window, e.getRectSelectionMode());
}

TEST(RectSelectionTest, testGetRectSelectionModeDragLeft) {
  GraphicsSceneMouseEvent e;
  e.downScreenPos = QPointF(10, 0);
  e.screenPos = QPointF(0, 0);
  EXPECT_EQ(RectSelection::Mode::Crossing, e.getRectSelectionMode());
}

TEST(RectSelectionTest, testGetRectSelectionModeVerticalDrag) {
  // x() >= downScreenPos.x() with equal x -> Window, per the documented
  // ">=" comparison (a purely vertical drag is treated as Window).
  GraphicsSceneMouseEvent e;
  e.downScreenPos = QPointF(5, 0);
  e.screenPos = QPointF(5, 50);
  EXPECT_EQ(RectSelection::Mode::Window, e.getRectSelectionMode());
}

TEST(RectSelectionTest, testGetRectSelectionModeMirrorIndependent) {
  // The whole point of using raw screen coordinates instead of scene
  // coordinates: the same screenPos/downScreenPos pair yields the same mode
  // regardless of whether the underlying view is mirrored, since mirroring
  // is never applied to these fields in the first place.
  GraphicsSceneMouseEvent e;
  e.downScreenPos = QPointF(20, 20);
  e.screenPos = QPointF(50, 20);
  EXPECT_EQ(RectSelection::Mode::Window, e.getRectSelectionMode());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
