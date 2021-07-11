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
#include <librepcb/projecteditor/boardeditor/boardclipboarddata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class BoardClipboardDataTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(BoardClipboardDataTest, testToFromMimeDataEmpty) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);

  // Create object
  BoardClipboardData obj1(uuid, pos);

  // Serialize to MIME data
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData();

  // Load from MIME data and validate
  std::unique_ptr<BoardClipboardData> obj2 =
      BoardClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getBoardUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getDevices(), obj2->getDevices());
  EXPECT_EQ(obj1.getNetSegments(), obj2->getNetSegments());
  EXPECT_EQ(obj1.getPlanes(), obj2->getPlanes());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getStrokeTexts(), obj2->getStrokeTexts());
  EXPECT_EQ(obj1.getHoles(), obj2->getHoles());
  EXPECT_EQ(obj1.getPadPositions(), obj2->getPadPositions());
}

TEST(BoardClipboardDataTest, testToFromMimeDataPopulated) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(12345, 54321);

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

  std::shared_ptr<BoardClipboardData::Device> device1 =
      std::make_shared<BoardClipboardData::Device>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          Point::fromMm(1, 2), Angle::fromDeg(45), false,
          StrokeTextList{strokeText1, strokeText2});

  std::shared_ptr<BoardClipboardData::Device> device2 =
      std::make_shared<BoardClipboardData::Device>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          Point::fromMm(10, 20), Angle::fromDeg(-45), true,
          StrokeTextList{strokeText2, strokeText1});

  std::shared_ptr<BoardClipboardData::NetSegment> netSegment1 =
      std::make_shared<BoardClipboardData::NetSegment>(
          CircuitIdentifier("net1"));
  netSegment1->vias.append(std::make_shared<Via>(
      Uuid::createRandom(), Point(1, 2), Via::Shape::Round, PositiveLength(10),
      PositiveLength(3)));
  netSegment1->vias.append(std::make_shared<Via>(
      Uuid::createRandom(), Point(10, 20), Via::Shape::Square,
      PositiveLength(100), PositiveLength(30)));
  netSegment1->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(1, 2)));
  netSegment1->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(3, 4)));
  netSegment1->traces.append(std::make_shared<Trace>(
      Uuid::createRandom(), GraphicsLayerName("foo"), PositiveLength(1),
      TraceAnchor::junction(Uuid::createRandom()),
      TraceAnchor::via(Uuid::createRandom())));
  netSegment1->traces.append(std::make_shared<Trace>(
      Uuid::createRandom(), GraphicsLayerName("bar"), PositiveLength(10),
      TraceAnchor::junction(Uuid::createRandom()),
      TraceAnchor::pad(Uuid::createRandom(), Uuid::createRandom())));

  std::shared_ptr<BoardClipboardData::NetSegment> netSegment2 =
      std::make_shared<BoardClipboardData::NetSegment>(
          CircuitIdentifier("net2"));
  netSegment2->vias.append(std::make_shared<Via>(
      Uuid::createRandom(), Point(1, 2), Via::Shape::Round, PositiveLength(10),
      PositiveLength(3)));
  netSegment2->vias.append(std::make_shared<Via>(
      Uuid::createRandom(), Point(10, 20), Via::Shape::Square,
      PositiveLength(100), PositiveLength(30)));
  netSegment2->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(1, 2)));
  netSegment2->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(3, 4)));
  netSegment2->traces.append(std::make_shared<Trace>(
      Uuid::createRandom(), GraphicsLayerName("foo"), PositiveLength(1),
      TraceAnchor::junction(Uuid::createRandom()),
      TraceAnchor::via(Uuid::createRandom())));
  netSegment2->traces.append(std::make_shared<Trace>(
      Uuid::createRandom(), GraphicsLayerName("bar"), PositiveLength(10),
      TraceAnchor::junction(Uuid::createRandom()),
      TraceAnchor::pad(Uuid::createRandom(), Uuid::createRandom())));

  std::shared_ptr<BoardClipboardData::Plane> plane1 =
      std::make_shared<BoardClipboardData::Plane>(
          Uuid::createRandom(), GraphicsLayerName("foo"),
          CircuitIdentifier("bar"),
          Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}),
          UnsignedLength(1), UnsignedLength(2), false, 0,
          BI_Plane::ConnectStyle::None);

  std::shared_ptr<BoardClipboardData::Plane> plane2 =
      std::make_shared<BoardClipboardData::Plane>(
          Uuid::createRandom(), GraphicsLayerName("bar"),
          CircuitIdentifier("foo"),
          Path({Vertex(Point(10, 20), Angle(30)),
                Vertex(Point(40, 50), Angle(60))}),
          UnsignedLength(10), UnsignedLength(20), true, 5,
          BI_Plane::ConnectStyle::Solid);

  std::shared_ptr<Polygon> polygon1 = std::make_shared<Polygon>(
      Uuid::createRandom(), GraphicsLayerName("foo"), UnsignedLength(1), false,
      true,
      Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}));

  std::shared_ptr<Polygon> polygon2 =
      std::make_shared<Polygon>(Uuid::createRandom(), GraphicsLayerName("bar"),
                                UnsignedLength(10), true, false,
                                Path({Vertex(Point(10, 20), Angle(30)),
                                      Vertex(Point(40, 50), Angle(60))}));

  std::shared_ptr<Hole> hole1 = std::make_shared<Hole>(
      Uuid::createRandom(), Point(1, 2), PositiveLength(3));

  std::shared_ptr<Hole> hole2 = std::make_shared<Hole>(
      Uuid::createRandom(), Point(10, 20), PositiveLength(30));

  // Create object
  BoardClipboardData obj1(uuid, pos);
  obj1.getDevices().append(device1);
  obj1.getDevices().append(device2);
  obj1.getNetSegments().append(netSegment1);
  obj1.getNetSegments().append(netSegment2);
  obj1.getPlanes().append(plane1);
  obj1.getPlanes().append(plane2);
  obj1.getPolygons().append(polygon1);
  obj1.getPolygons().append(polygon2);
  obj1.getStrokeTexts().append(strokeText1);
  obj1.getStrokeTexts().append(strokeText2);
  obj1.getHoles().append(hole1);
  obj1.getHoles().append(hole2);
  obj1.getPadPositions().insert(
      std::make_pair(Uuid::createRandom(), Uuid::createRandom()), Point(1, 2));
  obj1.getPadPositions().insert(
      std::make_pair(Uuid::createRandom(), Uuid::createRandom()), Point(3, 4));

  // Serialize to MIME data
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData();

  // Load from MIME data and validate
  std::unique_ptr<BoardClipboardData> obj2 =
      BoardClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getBoardUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getDevices(), obj2->getDevices());
  EXPECT_EQ(obj1.getNetSegments(), obj2->getNetSegments());
  EXPECT_EQ(obj1.getPlanes(), obj2->getPlanes());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getStrokeTexts(), obj2->getStrokeTexts());
  EXPECT_EQ(obj1.getHoles(), obj2->getHoles());
  EXPECT_EQ(obj1.getPadPositions(), obj2->getPadPositions());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace project
}  // namespace librepcb
