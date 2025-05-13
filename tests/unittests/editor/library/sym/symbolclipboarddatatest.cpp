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
#include <librepcb/core/types/layer.h>
#include <librepcb/editor/graphics/graphicslayerlist.h>
#include <librepcb/editor/library/sym/symbolclipboarddata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SymbolClipboardDataTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SymbolClipboardDataTest, testToFromMimeDataEmpty) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);

  // Create object
  SymbolClipboardData obj1(uuid, pos);

  // Serialize to MIME data
  std::unique_ptr<GraphicsLayerList> layers =
      GraphicsLayerList::previewLayers(nullptr);
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData(*layers);

  // Load from MIME data and validate
  std::unique_ptr<SymbolClipboardData> obj2 =
      SymbolClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getSymbolUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getPins(), obj2->getPins());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getCircles(), obj2->getCircles());
  EXPECT_EQ(obj1.getTexts(), obj2->getTexts());
}

TEST(SymbolClipboardDataTest, testToFromMimeDataPopulated) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);

  std::shared_ptr<SymbolPin> pin1 = std::make_shared<SymbolPin>(
      Uuid::createRandom(), CircuitIdentifier("foo"), Point(12, 34),
      UnsignedLength(0), Angle(56), Point(78, 90), Angle(98),
      PositiveLength(76), Alignment(HAlign::center(), VAlign::top()));

  std::shared_ptr<SymbolPin> pin2 = std::make_shared<SymbolPin>(
      Uuid::createRandom(), CircuitIdentifier("bar"), Point(120, 340),
      UnsignedLength(123), Angle(789), Point(987, 654), Angle(32),
      PositiveLength(10), Alignment(HAlign::right(), VAlign::bottom()));

  std::shared_ptr<Polygon> polygon1 = std::make_shared<Polygon>(
      Uuid::createRandom(), Layer::botCopper(), UnsignedLength(1), false, true,
      Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}));

  std::shared_ptr<Polygon> polygon2 = std::make_shared<Polygon>(
      Uuid::createRandom(), Layer::topLegend(), UnsignedLength(10), true, false,
      Path({Vertex(Point(10, 20), Angle(30)),
            Vertex(Point(40, 50), Angle(60))}));

  std::shared_ptr<Circle> circle1 = std::make_shared<Circle>(
      Uuid::createRandom(), Layer::botStopMask(), UnsignedLength(123), false,
      true, Point(12, 34), PositiveLength(1234));

  std::shared_ptr<Circle> circle2 = std::make_shared<Circle>(
      Uuid::createRandom(), Layer::topSolderPaste(), UnsignedLength(0), true,
      false, Point(120, 340), PositiveLength(12));

  std::shared_ptr<Text> text1 = std::make_shared<Text>(
      Uuid::createRandom(), Layer::topCopper(), "text 1", Point(1, 2), Angle(3),
      PositiveLength(4), Alignment(HAlign::left(), VAlign::top()));

  std::shared_ptr<Text> text2 =
      std::make_shared<Text>(Uuid::createRandom(), Layer::botCopper(), "text 2",
                             Point(10, 20), Angle(30), PositiveLength(40),
                             Alignment(HAlign::center(), VAlign::bottom()));

  // Create object
  SymbolClipboardData obj1(uuid, pos);
  obj1.getPins().append(pin1);
  obj1.getPins().append(pin2);
  obj1.getPolygons().append(polygon1);
  obj1.getPolygons().append(polygon2);
  obj1.getCircles().append(circle1);
  obj1.getCircles().append(circle2);
  obj1.getTexts().append(text1);
  obj1.getTexts().append(text2);

  // Serialize to MIME data
  std::unique_ptr<GraphicsLayerList> layers =
      GraphicsLayerList::previewLayers(nullptr);
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData(*layers);

  // Load from MIME data and validate
  std::unique_ptr<SymbolClipboardData> obj2 =
      SymbolClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getSymbolUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getPins(), obj2->getPins());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getCircles(), obj2->getCircles());
  EXPECT_EQ(obj1.getTexts(), obj2->getTexts());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
