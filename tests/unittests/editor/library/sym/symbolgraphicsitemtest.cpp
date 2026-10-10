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
#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/editor/graphics/graphicslayerlist.h>
#include <librepcb/editor/library/sym/symbolgraphicsitem.h>

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

// Builds a Symbol with one item of each child type that
// SymbolGraphicsItem::setSelectionRect() iterates, except images. Pin, circle,
// polygon, and text are spaced 15mm apart along the X axis so each can be
// targeted in isolation.
//
// The pin's shape() is an exact 1.2mm Qt::FilledOutline circle (see
// SymbolPinGraphicsItem), so it gets the full enclosed/partial/disjoint
// treatment with tight confidence. Circle & polygon use a simple filled
// primitive with a known nominal size, so a rect offset by roughly a
// millimeter from their true extents is safely inside/outside either way.
// The text's shape() is a PrimitiveTextGraphicsItem font-metrics box, whose
// exact extents are font dependent, so only the unambiguous enclosed case is
// asserted for it (same reasoning as the stroke text case in
// footprintgraphicsitemtest.cpp).
class SymbolGraphicsItemTest : public ::testing::Test {
protected:
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<Symbol> mSymbol;
  std::unique_ptr<SymbolGraphicsItem> mItem;

  void SetUp() override {
    mLayers = GraphicsLayerList::libraryLayers(nullptr);

    mSymbol = std::make_unique<Symbol>(
        Uuid::createRandom(), Version::fromString("0.1"), "Test",
        QDateTime::currentDateTime(), ElementName("Symbol"), "", "");

    // Pin: 2.54mm long, positioned at X=0. Its shape() is just the 1.2mm
    // circle at the pin's own position (see class comment above).
    mSymbol->getPins().append(std::make_shared<SymbolPin>(
        Uuid::createRandom(), CircuitIdentifier("1"), Point(0, 5000000),
        UnsignedLength(2540000), Angle::deg0(), Point(3000000, 5000000),
        Angle::deg0(), PositiveLength(2000000), Alignment()));

    // Circle: 1mm diameter, filled, centered at X=15mm.
    mSymbol->getCircles().append(std::make_shared<Circle>(
        Uuid::createRandom(), Layer::symbolOutlines(), UnsignedLength(0),
        true, false, Point(15000000, 0), PositiveLength(1000000)));

    // Polygon: 1x1mm filled square, centered at X=30mm.
    mSymbol->getPolygons().append(std::make_shared<Polygon>(
        Uuid::createRandom(), Layer::symbolOutlines(), UnsignedLength(0),
        true, false,
        Path::rect(Point(29500000, -500000), Point(30500000, 500000))));

    // Text: anchored at X=45mm.
    mSymbol->getTexts().append(std::make_shared<Text>(
        Uuid::createRandom(), Layer::symbolOutlines(), "A",
        Point(45000000, 0), Angle::deg0(), PositiveLength(1000000),
        Alignment(), false));

    mItem = std::make_unique<SymbolGraphicsItem>(
        *mSymbol, *mLayers, QPointer<const Component>(), nullptr,
        QStringList(), false);
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

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectEmptyRectSelectsNothing) {
  mItem->setSelectionRect(QRectF(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPins().isEmpty());
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  EXPECT_TRUE(mItem->getSelectedTexts().isEmpty());
}

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectFarAwayRectSelectsNothing) {
  // Crossing is the more inclusive mode, so proving it selects nothing here
  // also proves Window would select nothing.
  mItem->setSelectionRect(disjointRect(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPins().isEmpty());
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  EXPECT_TRUE(mItem->getSelectedTexts().isEmpty());
}

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectReplacesPreviousSelection) {
  // Selection is replace, not additive: re-calling with a non-matching rect
  // must deselect whatever the previous call selected.
  mItem->setSelectionRect(rect(Point(-1200000, 3800000), Point(1200000, 6200000)),
                          RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPins().size());

  mItem->setSelectionRect(QRectF(), RectSelection::Mode::Crossing);
  EXPECT_TRUE(mItem->getSelectedPins().isEmpty());
}

/*******************************************************************************
 *  Test Methods: pin
 ******************************************************************************/

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectPinFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(-1200000, 3800000), Point(1200000, 6200000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPins().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPins().size());
}

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectPinPartialOverlapSelectsOnlyInCrossingMode) {
  // Covers only the right half of the pin's 1.2mm circle, missing its left
  // half entirely.
  const QRectF r = rect(Point(0, 3800000), Point(1200000, 6200000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedPins().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPins().size());
}

/*******************************************************************************
 *  Test Methods: circle
 ******************************************************************************/

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectCircleFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(13500000, -1500000), Point(16500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
}

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectCirclePartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(15000000, -1500000), Point(16500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedCircles().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedCircles().size());
}

/*******************************************************************************
 *  Test Methods: polygon
 ******************************************************************************/

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectPolygonFullyEnclosedSelectsInBothModes) {
  const QRectF r = rect(Point(28500000, -1500000), Point(31500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
}

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectPolygonPartialOverlapSelectsOnlyInCrossingMode) {
  const QRectF r = rect(Point(30000000, -1500000), Point(31500000, 1500000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_TRUE(mItem->getSelectedPolygons().isEmpty());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedPolygons().size());
}

/*******************************************************************************
 *  Test Methods: text
 ******************************************************************************/

TEST_F(SymbolGraphicsItemTest, testSetSelectionRectTextFullyEnclosedSelectsInBothModes) {
  // Generous margin around the anchor position: see the class comment above
  // for why only the enclosed/disjoint cases are asserted for text.
  const QRectF r = rect(Point(42000000, -3000000), Point(48000000, 3000000));
  mItem->setSelectionRect(r, RectSelection::Mode::Window);
  EXPECT_EQ(1, mItem->getSelectedTexts().size());
  mItem->setSelectionRect(r, RectSelection::Mode::Crossing);
  EXPECT_EQ(1, mItem->getSelectedTexts().size());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
