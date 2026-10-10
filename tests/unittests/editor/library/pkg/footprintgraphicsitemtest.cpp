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
#include <librepcb/core/application.h>
#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/editor/graphics/graphicslayerlist.h>
#include <librepcb/editor/library/pkg/footprintgraphicsitem.h>

#include <QtCore>

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

// Builds a Footprint with one item of each child type that
// FootprintGraphicsItem::setSelectionRect() iterates (pad, circle, polygon,
// stroke text, zone, hole), spaced 15mm apart along the X axis. This enables
// us to target each one in isolation without the containment margins ever
// reaching a neighboring item.
//
// Window-mode containment is only asserted against the pad, circle, polygon,
// zone, and hole: their shapes are either an exact Qt::FilledOutline shape
// (the pad) or a simple filled primitive with a known nominal size. The stroke
// text's shape is the actual stroked glyph outline from the StrokeFont, whose
// exact ink extents are font/glyph dependent.  Thus, only the unambiguous
// enclosed/disjoint cases are asserted for it (see 
// testSetSelectionRectStrokeTextFullyEnclosedSelectsInBothModes).
class FootprintGraphicsItemTest : public ::testing::Test {
protected:
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::shared_ptr<Footprint> mFootprint;
  std::unique_ptr<FootprintGraphicsItem> mItem;

  void SetUp() override {
    mLayers = GraphicsLayerList::libraryLayers(nullptr);

    mFootprint = std::make_shared<Footprint>(
        Uuid::createRandom(), ElementName("Footprint"), "");

    // Pad: 1x1mm rounded rect, centered at X=0.
    mFootprint->getPads().append(std::make_shared<FootprintPad>(
        Uuid::createRandom(), std::nullopt, Point(0, 5000000), Angle::deg0(),
        Pad::Shape::RoundedRect, PositiveLength(1000000),
        PositiveLength(1000000), UnsignedLimitedRatio(Ratio::fromPercent(0)),
        Path(), MaskConfig::automatic(), MaskConfig::automatic(),
        UnsignedLength(0), Pad::ComponentSide::Top, Pad::Function::Unspecified,
        PadHoleList()));

    // Circle: 1mm diameter, filled, centered at X=15mm.
    mFootprint->getCircles().append(std::make_shared<Circle>(
        Uuid::createRandom(), Layer::topDocumentation(), UnsignedLength(0),
        true, false, Point(15000000, 0), PositiveLength(1000000)));

    // Polygon: 1x1mm filled square, centered at X=30mm.
    mFootprint->getPolygons().append(std::make_shared<Polygon>(
        Uuid::createRandom(), Layer::topDocumentation(), UnsignedLength(0),
        true, false,
        Path::rect(Point(29500000, -500000), Point(30500000, 500000))));

    // Stroke text: single glyph anchored at X=45mm.
    mFootprint->getStrokeTexts().append(std::make_shared<StrokeText>(
        Uuid::createRandom(), Layer::topDocumentation(), "A",
        Point(45000000, 0), Angle::deg0(), PositiveLength(1000000),
        UnsignedLength(200000), StrokeTextSpacing(), StrokeTextSpacing(),
        Alignment(), false, false, false));

    // Zone: 1x1mm square, centered at X=60mm.
    mFootprint->getZones().append(std::make_shared<Zone>(
        Uuid::createRandom(), Zone::Layers(Zone::Layer::Top),
        Zone::Rules(Zone::Rule::NoCopper),
        Path::rect(Point(59500000, -500000), Point(60500000, 500000))));

    // Hole: 0.5mm diameter, centered at X=75mm.
    mFootprint->getHoles().append(std::make_shared<Hole>(
        Uuid::createRandom(), PositiveLength(500000),
        NonEmptyPath(Path::circle(PositiveLength(500000))
                        .translated(Point(75000000, 0))),
        MaskConfig::off()));

    mItem = std::make_unique<FootprintGraphicsItem>(
        mFootprint, *mLayers, Application::getDefaultStrokeFont());
  }

  static QRectF rect(const Point& p1, const Point& p2) noexcept {
    return QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  }

  // Far from every item above regardless of which one is being tested.
  static QRectF disjointRect() noexcept {
    return rect(Point(500000000, 500000000), Point(501000000, 501000000));
  }
};

/*******************************************************************************
 *  Test Methods: general wiring
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectEmptyRectSelectsNothing) {
  // Mirrors the real call sites that clear highlighting with an empty rect
  // (e.g. DeviceTab), and doubles as a baseline that every container starts
  // out unselected.
  mItem->setSelectionRect(QRectF(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPads().isEmpty());
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  EXPECT_TRUE(mItem->getSelectedStrokeTexts().isEmpty());
  EXPECT_TRUE(mItem->getSelectedZones().isEmpty());
  EXPECT_TRUE(mItem->getSelectedHoles().isEmpty());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectFarAwayRectSelectsNothing) {
  // Crossing is the more inclusive mode, so proving it selects nothing here
  // also proves Window would select nothing.
  mItem->setSelectionRect(disjointRect(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPads().isEmpty());
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  EXPECT_TRUE(mItem->getSelectedStrokeTexts().isEmpty());
  EXPECT_TRUE(mItem->getSelectedZones().isEmpty());
  EXPECT_TRUE(mItem->getSelectedHoles().isEmpty());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectReplacesPreviousSelection) {
  // Selection is replace, not additive: re-calling with a non-matching rect
  // must deselect whatever the previous call selected.
  mItem->setSelectionRect(rect(Point(-1500000, 3500000), Point(1500000, 6500000)),
                          RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPads().size());

  mItem->setSelectionRect(QRectF(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPads().isEmpty());
}

/*******************************************************************************
 *  Test Methods: pad
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectPadFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(-1500000, 3500000), Point(1500000, 6500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPads().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPads().size());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectPadPartialOverlapSelectsOnlyInCrossingMode) {
  // Covers only the right half of the pad (plus margin), missing its left
  // half entirely.
  const QRectF r = rect(Point(0, 3500000), Point(1500000, 6500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedPads().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPads().size());
}

/*******************************************************************************
 *  Test Methods: circle
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectCircleFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(13500000, -1500000), Point(16500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectCirclePartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(15000000, -1500000), Point(16500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
}

/*******************************************************************************
 *  Test Methods: polygon
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectPolygonFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(28500000, -1500000), Point(31500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectPolygonPartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(30000000, -1500000), Point(31500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
}

/*******************************************************************************
 *  Test Methods: stroke text
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectStrokeTextFullyEnclosedSelectsInBothModes) {
  // Generous margin around the anchor position: see the class comment above
  // for why only the enclosed/disjoint cases are asserted for stroke text.
  const QRectF r = rect(Point(42000000, -3000000), Point(48000000, 3000000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedStrokeTexts().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedStrokeTexts().size());
}

/*******************************************************************************
 *  Test Methods: zone
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectZoneFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(58500000, -1500000), Point(61500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedZones().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedZones().size());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectZonePartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(60000000, -1500000), Point(61500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedZones().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedZones().size());
}

/*******************************************************************************
 *  Test Methods: hole
 ******************************************************************************/

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectHoleFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(73500000, -1500000), Point(76500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedHoles().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedHoles().size());
}

TEST_F(FootprintGraphicsItemTest, testSetSelectionRectHolePartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(75000000, -1500000), Point(76500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedHoles().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedHoles().size());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
