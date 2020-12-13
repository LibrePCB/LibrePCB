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
#include <librepcb/common/attributes/attrtypestring.h>
#include <librepcb/common/attributes/attrtypevoltage.h>
#include <librepcb/projecteditor/schematiceditor/schematicclipboarddata.h>

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

class SchematicClipboardDataTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SchematicClipboardDataTest, testToFromMimeDataEmpty) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(Length(12345), Length(54321));

  // Create object
  SchematicClipboardData obj1(uuid, pos);

  // Serialize to MIME data
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData();

  // Load from MIME data and validate
  std::unique_ptr<SchematicClipboardData> obj2 =
      SchematicClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getSchematicUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getComponentInstances(), obj2->getComponentInstances());
  EXPECT_EQ(obj1.getNetSegments(), obj2->getNetSegments());
  EXPECT_EQ(obj1.getSymbolInstances(), obj2->getSymbolInstances());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getTexts(), obj2->getTexts());
}

TEST(SchematicClipboardDataTest, testToFromMimeDataPopulated) {
  // Create data
  Uuid uuid = Uuid::createRandom();
  Point pos(Length(12345), Length(54321));

  std::shared_ptr<Attribute> attribute1 = std::make_shared<Attribute>(
      AttributeKey("A1"), AttrTypeString::instance(), "foo bar", nullptr);

  std::shared_ptr<Attribute> attribute2 = std::make_shared<Attribute>(
      AttributeKey("A2"), AttrTypeVoltage::instance(), "4.2",
      AttrTypeVoltage::instance().getUnitFromString("millivolt"));

  std::shared_ptr<SchematicClipboardData::ComponentInstance> component1 =
      std::make_shared<SchematicClipboardData::ComponentInstance>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          Uuid::createRandom(), CircuitIdentifier("foo"), "bar",
          AttributeList{attribute1, attribute2});

  std::shared_ptr<SchematicClipboardData::ComponentInstance> component2 =
      std::make_shared<SchematicClipboardData::ComponentInstance>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          tl::nullopt, CircuitIdentifier("bar"), "hello world",
          AttributeList{attribute2, attribute1});

  std::shared_ptr<SchematicClipboardData::SymbolInstance> symbol1 =
      std::make_shared<SchematicClipboardData::SymbolInstance>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          Point(123, 456), Angle(789), false);

  std::shared_ptr<SchematicClipboardData::SymbolInstance> symbol2 =
      std::make_shared<SchematicClipboardData::SymbolInstance>(
          Uuid::createRandom(), Uuid::createRandom(), Uuid::createRandom(),
          Point(321, 987), Angle(555), true);

  std::shared_ptr<SchematicClipboardData::NetSegment> netSegment1 =
      std::make_shared<SchematicClipboardData::NetSegment>(
          CircuitIdentifier("net1"));
  netSegment1->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(1, 2)));
  netSegment1->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(3, 4)));
  netSegment1->lines.append(std::make_shared<NetLine>(
      Uuid::createRandom(), UnsignedLength(1),
      NetLineAnchor::junction(Uuid::createRandom()),
      NetLineAnchor::pin(Uuid::createRandom(), Uuid::createRandom())));
  netSegment1->lines.append(std::make_shared<NetLine>(
      Uuid::createRandom(), UnsignedLength(0),
      NetLineAnchor::junction(Uuid::createRandom()),
      NetLineAnchor::pin(Uuid::createRandom(), Uuid::createRandom())));
  netSegment1->labels.append(std::make_shared<NetLabel>(
      Uuid::createRandom(), Point(12, 34), Angle(56)));
  netSegment1->labels.append(std::make_shared<NetLabel>(
      Uuid::createRandom(), Point(123, 456), Angle(789)));

  std::shared_ptr<SchematicClipboardData::NetSegment> netSegment2 =
      std::make_shared<SchematicClipboardData::NetSegment>(
          CircuitIdentifier("net2"));
  netSegment2->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(10, 20)));
  netSegment2->junctions.append(
      std::make_shared<Junction>(Uuid::createRandom(), Point(30, 40)));
  netSegment2->lines.append(
      std::make_shared<NetLine>(Uuid::createRandom(), UnsignedLength(10),
                                NetLineAnchor::junction(Uuid::createRandom()),
                                NetLineAnchor::junction(Uuid::createRandom())));
  netSegment2->lines.append(std::make_shared<NetLine>(
      Uuid::createRandom(), UnsignedLength(100),
      NetLineAnchor::junction(Uuid::createRandom()),
      NetLineAnchor::pin(Uuid::createRandom(), Uuid::createRandom())));
  netSegment2->labels.append(std::make_shared<NetLabel>(
      Uuid::createRandom(), Point(120, 340), Angle(560)));
  netSegment2->labels.append(std::make_shared<NetLabel>(
      Uuid::createRandom(), Point(1230, 4560), Angle(7890)));

  std::shared_ptr<Polygon> polygon1 = std::make_shared<Polygon>(
      Uuid::createRandom(), GraphicsLayerName("foo"), UnsignedLength(1), false,
      true,
      Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}));

  std::shared_ptr<Polygon> polygon2 =
      std::make_shared<Polygon>(Uuid::createRandom(), GraphicsLayerName("bar"),
                                UnsignedLength(10), true, false,
                                Path({Vertex(Point(10, 20), Angle(30)),
                                      Vertex(Point(40, 50), Angle(60))}));

  std::shared_ptr<Text> text1 = std::make_shared<Text>(
      Uuid::createRandom(), GraphicsLayerName("foo"), "text 1", Point(1, 2),
      Angle(3), PositiveLength(4), Alignment(HAlign::left(), VAlign::top()));

  std::shared_ptr<Text> text2 = std::make_shared<Text>(
      Uuid::createRandom(), GraphicsLayerName("bar"), "text 2", Point(10, 20),
      Angle(30), PositiveLength(40),
      Alignment(HAlign::center(), VAlign::bottom()));

  // Create object
  SchematicClipboardData obj1(uuid, pos);
  obj1.getComponentInstances().append(component1);
  obj1.getComponentInstances().append(component2);
  obj1.getSymbolInstances().append(symbol1);
  obj1.getSymbolInstances().append(symbol2);
  obj1.getNetSegments().append(netSegment1);
  obj1.getNetSegments().append(netSegment2);
  obj1.getPolygons().append(polygon1);
  obj1.getPolygons().append(polygon2);
  obj1.getTexts().append(text1);
  obj1.getTexts().append(text2);

  // Serialize to MIME data
  std::unique_ptr<QMimeData> mime1 = obj1.toMimeData();

  // Load from MIME data and validate
  std::unique_ptr<SchematicClipboardData> obj2 =
      SchematicClipboardData::fromMimeData(mime1.get());
  EXPECT_EQ(uuid, obj2->getSchematicUuid());
  EXPECT_EQ(pos, obj2->getCursorPos());
  EXPECT_EQ(obj1.getComponentInstances(), obj2->getComponentInstances());
  EXPECT_EQ(obj1.getNetSegments(), obj2->getNetSegments());
  EXPECT_EQ(obj1.getSymbolInstances(), obj2->getSymbolInstances());
  EXPECT_EQ(obj1.getPolygons(), obj2->getPolygons());
  EXPECT_EQ(obj1.getTexts(), obj2->getTexts());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace project
}  // namespace librepcb
