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
#include <librepcb/common/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/libraryeditor/pkg/footprintclipboarddata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FootprintClipboardDataTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(FootprintClipboardDataTest, testToFromMimeDataEmpty) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);
  PackagePadList packagePads;

  // Create object
  FootprintClipboardData obj1(uuid, packagePads, pos);

  // Serialize to MIME data
  DefaultGraphicsLayerProvider layerProvider;
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData(layerProvider);

  // Load from MIME data and validate
  std::unique_ptr<FootprintClipboardData> obj2 =
      FootprintClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getFootprintUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getPackagePads(), obj2->getPackagePads());
  EXPECT_EQ(obj1.getFootprintPads(), obj2->getFootprintPads());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getCircles(), obj2->getCircles());
  EXPECT_EQ(obj1.getStrokeTexts(), obj2->getStrokeTexts());
  EXPECT_EQ(obj1.getHoles(), obj2->getHoles());
}

TEST(FootprintClipboardDataTest, testToFromMimeDataPopulated) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);
  PackagePadList packagePads;

  std::shared_ptr<PackagePad> packagePad1 = std::make_shared<PackagePad>(
      Uuid::createRandom(), CircuitIdentifier("pad1"));

  std::shared_ptr<PackagePad> packagePad2 = std::make_shared<PackagePad>(
      Uuid::createRandom(), CircuitIdentifier("pad2"));

  std::shared_ptr<FootprintPad> footprintPad1 = std::make_shared<FootprintPad>(
      Uuid::createRandom(), Point(12, 34), Angle(56),
      FootprintPad::Shape::OCTAGON, PositiveLength(11), PositiveLength(22),
      UnsignedLength(0), FootprintPad::BoardSide::TOP);

  std::shared_ptr<FootprintPad> footprintPad2 = std::make_shared<FootprintPad>(
      Uuid::createRandom(), Point(12, 34), Angle(56), FootprintPad::Shape::RECT,
      PositiveLength(123), PositiveLength(456), UnsignedLength(789),
      FootprintPad::BoardSide::THT);

  std::shared_ptr<Polygon> polygon1 = std::make_shared<Polygon>(
      Uuid::createRandom(), GraphicsLayerName("foo"), UnsignedLength(1), false,
      true,
      Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}));

  std::shared_ptr<Polygon> polygon2 =
      std::make_shared<Polygon>(Uuid::createRandom(), GraphicsLayerName("bar"),
                                UnsignedLength(10), true, false,
                                Path({Vertex(Point(10, 20), Angle(30)),
                                      Vertex(Point(40, 50), Angle(60))}));

  std::shared_ptr<Circle> circle1 = std::make_shared<Circle>(
      Uuid::createRandom(), GraphicsLayerName("foo"), UnsignedLength(123),
      false, true, Point(12, 34), PositiveLength(1234));

  std::shared_ptr<Circle> circle2 = std::make_shared<Circle>(
      Uuid::createRandom(), GraphicsLayerName("bar"), UnsignedLength(0), true,
      false, Point(120, 340), PositiveLength(12));

  std::shared_ptr<StrokeText> strokeText1 = std::make_shared<StrokeText>(
      Uuid::createRandom(), GraphicsLayerName("foo"), "text 1", Point(1, 2),
      Angle(3), PositiveLength(4), UnsignedLength(5), StrokeTextSpacing(),
      StrokeTextSpacing(Ratio(6)), Alignment(HAlign::left(), VAlign::top()),
      false, true);

  std::shared_ptr<StrokeText> strokeText2 = std::make_shared<StrokeText>(
      Uuid::createRandom(), GraphicsLayerName("bar"), "text 2", Point(10, 20),
      Angle(30), PositiveLength(40), UnsignedLength(0),
      StrokeTextSpacing(Ratio(6)), StrokeTextSpacing(),
      Alignment(HAlign::center(), VAlign::bottom()), true, false);

  std::shared_ptr<Hole> hole1 = std::make_shared<Hole>(
      Uuid::createRandom(), Point(1, 2), PositiveLength(3));

  std::shared_ptr<Hole> hole2 = std::make_shared<Hole>(
      Uuid::createRandom(), Point(10, 20), PositiveLength(30));

  // Create object
  FootprintClipboardData obj1(uuid, packagePads, pos);
  obj1.getPackagePads().append(packagePad1);
  obj1.getPackagePads().append(packagePad2);
  obj1.getFootprintPads().append(footprintPad1);
  obj1.getFootprintPads().append(footprintPad2);
  obj1.getPolygons().append(polygon1);
  obj1.getPolygons().append(polygon2);
  obj1.getCircles().append(circle1);
  obj1.getCircles().append(circle2);
  obj1.getStrokeTexts().append(strokeText1);
  obj1.getStrokeTexts().append(strokeText2);
  obj1.getHoles().append(hole1);
  obj1.getHoles().append(hole2);

  // Serialize to MIME data
  DefaultGraphicsLayerProvider layerProvider;
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData(layerProvider);

  // Load from MIME data and validate
  std::unique_ptr<FootprintClipboardData> obj2 =
      FootprintClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getFootprintUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getPackagePads(), obj2->getPackagePads());
  EXPECT_EQ(obj1.getFootprintPads(), obj2->getFootprintPads());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getCircles(), obj2->getCircles());
  EXPECT_EQ(obj1.getStrokeTexts(), obj2->getStrokeTexts());
  EXPECT_EQ(obj1.getHoles(), obj2->getHoles());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace library
}  // namespace librepcb
