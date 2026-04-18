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
#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/packagepad.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/eagleimport/eagletypeconverter.h>
#include <parseagle/common/attribute.h>
#include <parseagle/common/domelement.h>
#include <parseagle/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class EagleTypeConverterTest : public ::testing::Test {
protected:
  static parseagle::DomElement dom(const QString& str) {
    return parseagle::DomElement::parse(str);
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EagleTypeConverterTest, testConvertElementName) {
  EagleTypeConverter c;
  EXPECT_EQ("Valid Name", c.convertElementName("Valid Name")->toStdString());
  EXPECT_EQ("X", c.convertElementName(" \nX ")->toStdString());
  EXPECT_EQ("Unnamed", c.convertElementName("\n")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertElementDescription) {
  EagleTypeConverter c;
  EXPECT_EQ("", c.convertElementDescription("").toStdString());
  EXPECT_EQ("Text", c.convertElementDescription(" Text ").toStdString());
  EXPECT_EQ("X\nY", c.convertElementDescription("X\nY").toStdString());
  EXPECT_EQ("X\nY",
            c.convertElementDescription("<b>X</b><br/>Y").toStdString());
  EXPECT_EQ("X\nY",
            c.convertElementDescription("<b>X</b>\n<br/>Y").toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertComponentName) {
  EagleTypeConverter c;
  EXPECT_EQ("Valid Name", c.convertComponentName("Valid Name")->toStdString());
  EXPECT_EQ("X", c.convertComponentName(" \nX ")->toStdString());
  EXPECT_EQ("Foo - Bar", c.convertComponentName("Foo - Bar-")->toStdString());
  EXPECT_EQ("Foo _ Bar", c.convertComponentName("Foo _ Bar_")->toStdString());
  EXPECT_EQ("-", c.convertComponentName("-")->toStdString());
  EXPECT_EQ("Unnamed", c.convertComponentName("\n")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertDeviceName) {
  EagleTypeConverter c;
  EXPECT_EQ("Valid Name", c.convertDeviceName("Valid Name", "")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            c.convertDeviceName("Valid Name", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            c.convertDeviceName("Valid Name-", "Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            c.convertDeviceName("Valid Name_", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            c.convertDeviceName("Valid Name-", "Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            c.convertDeviceName("Valid Name_", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            c.convertDeviceName("Valid Name", "-Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            c.convertDeviceName("Valid Name", "_Foo")->toStdString());
  EXPECT_EQ("X", c.convertDeviceName(" \nX ", "")->toStdString());
  EXPECT_EQ("Unnamed", c.convertDeviceName("\n", "")->toStdString());
  EXPECT_EQ("Unnamed", c.convertDeviceName("", "")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertComponentPrefix) {
  EagleTypeConverter c;
  EXPECT_EQ("", c.convertComponentPrefix("")->toStdString());
  EXPECT_EQ("", c.convertComponentPrefix("$42+")->toStdString());
  EXPECT_EQ("C", c.convertComponentPrefix("C")->toStdString());
  EXPECT_EQ("Foo_Bar", c.convertComponentPrefix(" Foo Bar ")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertGateName) {
  EagleTypeConverter c;
  EXPECT_EQ("", c.convertGateName("")->toStdString());
  EXPECT_EQ("G42", c.convertGateName("G$42")->toStdString());
  EXPECT_EQ("1", c.convertGateName("-1")->toStdString());
  EXPECT_EQ("Foo_Bar", c.convertGateName(" Foo Bar ")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertPinOrPadName) {
  EagleTypeConverter c;
  EXPECT_EQ("Unnamed", c.convertPinOrPadName(" ")->toStdString());
  EXPECT_EQ("42", c.convertPinOrPadName("P$42")->toStdString());
  EXPECT_EQ("3", c.convertPinOrPadName("3")->toStdString());
  EXPECT_EQ("Foo_Bar", c.convertPinOrPadName(" Foo Bar ")->toStdString());
  EXPECT_EQ("!FOO!/BAR", c.convertPinOrPadName("!FOO/BAR")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertInversionSyntax) {
  EagleTypeConverter c;
  EXPECT_EQ("FOO", c.convertInversionSyntax("FOO").toStdString());
  EXPECT_EQ("!FOO", c.convertInversionSyntax("!FOO").toStdString());
  EXPECT_EQ("!FOO", c.convertInversionSyntax("!FOO!").toStdString());
  EXPECT_EQ("!FOO/BAR", c.convertInversionSyntax("!FOO!/BAR").toStdString());
  EXPECT_EQ("!FOO!/BAR", c.convertInversionSyntax("!FOO/BAR").toStdString());
  EXPECT_EQ("FOO/!BAR", c.convertInversionSyntax("FOO/!BAR").toStdString());
  EXPECT_EQ("FOO/!BAR", c.convertInversionSyntax("FOO/!BAR!").toStdString());
  EXPECT_EQ("A/!B/C", c.convertInversionSyntax("A/!B!/C").toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertAttributeValid) {
  EagleTypeConverter c;
  MessageLogger log;
  const QString xml = "<attribute name=\"Foo Bar\" value=\"hello world!\"/>";
  auto out = c.tryConvertAttribute(parseagle::Attribute(dom(xml)), log);
  ASSERT_TRUE(out != nullptr);
  EXPECT_EQ("FOO_BAR", out->getKey()->toStdString());
  EXPECT_EQ("hello world!", out->getValue().toStdString());
  EXPECT_EQ(AttributeType::Type_t::String, out->getType().getType());
  EXPECT_EQ(0, log.getMessages().count());
}

TEST_F(EagleTypeConverterTest, testConvertAttributeInvalid) {
  EagleTypeConverter c;
  MessageLogger log;
  const QString xml = "<attribute name=\"!\" value=\"hello world!\"/>";
  auto out = c.tryConvertAttribute(parseagle::Attribute(dom(xml)), log);
  EXPECT_TRUE(out == nullptr);
  EXPECT_EQ(1, log.getMessages().count());
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicLayer) {
  EagleTypeConverter c;
  EXPECT_EQ(nullptr, c.tryConvertSchematicLayer(1));  // tCu
  EXPECT_EQ(&Layer::symbolOutlines(), c.tryConvertSchematicLayer(94));  // sym
  EXPECT_EQ(nullptr, c.tryConvertSchematicLayer(999));  // non existent
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardLayer) {
  EagleTypeConverter c;
  EXPECT_EQ(&Layer::topCopper(), c.tryConvertBoardLayer(1));  // tCu
  EXPECT_EQ(Layer::innerCopper(2), c.tryConvertBoardLayer(3));  // inner 2
  EXPECT_EQ(&Layer::botCopper(), c.tryConvertBoardLayer(16));  // bCu
  EXPECT_EQ(nullptr, c.tryConvertBoardLayer(94));  // symbols
  EXPECT_EQ(nullptr, c.tryConvertBoardLayer(999));  // non existent
}

TEST_F(EagleTypeConverterTest, testConvertLayerSetup) {
  EagleTypeConverter c;
  typedef QHash<const Layer*, const Layer*> T;
  EXPECT_EQ((T{}), c.convertLayerSetup(""));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            c.convertLayerSetup("1*16"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            c.convertLayerSetup("(1*16)"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {Layer::innerCopper(1), Layer::innerCopper(1)},
                {Layer::innerCopper(2), Layer::innerCopper(2)},
                {Layer::innerCopper(13), Layer::innerCopper(3)},
                {Layer::innerCopper(14), Layer::innerCopper(4)},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            c.convertLayerSetup("[2:1+((2*3)+(14*15))+16:15]"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {Layer::innerCopper(1), Layer::innerCopper(1)},
                {Layer::innerCopper(2), Layer::innerCopper(2)},
                {Layer::innerCopper(3), Layer::innerCopper(3)},
                {Layer::innerCopper(4), Layer::innerCopper(4)},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            c.convertLayerSetup("[2:1+[3:2+(3*4)+5:4]+16:5]"));
  EXPECT_THROW(c.convertLayerSetup("1*Foo*16"), Exception);
}

TEST_F(EagleTypeConverterTest, testConvertAlignment) {
  EagleTypeConverter c;
  EXPECT_EQ(Qt::AlignBottom | Qt::AlignRight,
            c.convertAlignment(parseagle::Alignment::BottomRight).toQtAlign());
  EXPECT_EQ(Qt::AlignTop | Qt::AlignHCenter,
            c.convertAlignment(parseagle::Alignment::TopCenter).toQtAlign());
}

TEST_F(EagleTypeConverterTest, testConvertLength) {
  EagleTypeConverter c;
  EXPECT_EQ(Length(0), c.convertLength(0));
  EXPECT_EQ(Length(-1234567), c.convertLength(-1.234567));
  EXPECT_EQ(Length(1234567), c.convertLength(1.234567));
}

TEST_F(EagleTypeConverterTest, testConvertLineWidth) {
  EagleTypeConverter c;
  EXPECT_EQ(UnsignedLength(0), c.convertLineWidth(0, 20));  // dimension
  EXPECT_EQ(UnsignedLength(0), c.convertLineWidth(0, 46));  // milling
  EXPECT_EQ(UnsignedLength(0), c.convertLineWidth(1.23, 20));  // dimension
  EXPECT_EQ(UnsignedLength(0), c.convertLineWidth(1.23, 46));  // milling
  EXPECT_EQ(UnsignedLength(1230000), c.convertLineWidth(1.23, 1));  // tCu
  EXPECT_EQ(UnsignedLength(1230000), c.convertLineWidth(1.23, 94));  // symbols
  EXPECT_THROW(c.convertLineWidth(-1.23, 94), Exception);
}

TEST_F(EagleTypeConverterTest, testConvertPoint) {
  EagleTypeConverter c;
  EXPECT_EQ(Point(0, 0), c.convertPoint(parseagle::Point{0, 0}));
  EXPECT_EQ(Point(-1234567, 1234567),
            c.convertPoint(parseagle::Point{-1.234567, 1.234567}));
}

TEST_F(EagleTypeConverterTest, testConvertAngle) {
  EagleTypeConverter c;
  EXPECT_EQ(Angle(0), c.convertAngle(0));
  EXPECT_EQ(Angle(-1234567), c.convertAngle(-1.234567));
  EXPECT_EQ(Angle(1234567), c.convertAngle(1.234567));
}

TEST_F(EagleTypeConverterTest, testConvertVertex) {
  EagleTypeConverter c;
  EXPECT_EQ(
      Vertex(Point(0, 0), Angle(0)),
      c.convertVertex(parseagle::Vertex(dom("<vertex x=\"0\" y=\"0\"/>"))));
  EXPECT_EQ(Vertex(Point(-6350000, 2540000), Angle(90000000)),
            c.convertVertex(parseagle::Vertex(
                dom("<vertex x=\"-6.35\" y=\"2.54\" curve=\"90\"/>"))));
}

TEST_F(EagleTypeConverterTest, testConvertVertices) {
  EagleTypeConverter c;
  QList<parseagle::Vertex> vertices{
      parseagle::Vertex(dom("<vertex x=\"-45.72\" y=\"-5.08\" curve=\"45\"/>")),
      parseagle::Vertex(dom("<vertex x=\"-35.56\" y=\"-5.08\"/>")),
      parseagle::Vertex(dom("<vertex x=\"-38.1\" y=\"-12.7\"/>")),
  };
  Path expected({
      Vertex(Point(-45720000, -5080000), Angle(45000000)),
      Vertex(Point(-35560000, -5080000), Angle(0)),
      Vertex(Point(-38100000, -12700000), Angle(0)),
      Vertex(Point(-45720000, -5080000), Angle(0)),
  });
  EXPECT_EQ(expected, c.convertVertices(vertices, true));
}

TEST_F(EagleTypeConverterTest, testConvertAndJoinWires) {
  EagleTypeConverter c;
  MessageLogger log;
  QList<parseagle::Wire> wires{
      // clang-format off
      parseagle::Wire(dom("<wire x1=\"1\" y1=\"2\" x2=\"3\" y2=\"4\" width=\"0.254\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"3\" y1=\"4\" x2=\"5\" y2=\"6\" width=\"0.254\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"5\" y1=\"6\" x2=\"7\" y2=\"8\" width=\"0.567\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"7\" y1=\"8\" x2=\"9\" y2=\"8\" width=\"0.567\" layer=\"1\" cap=\"flat\"/>")),
      parseagle::Wire(dom("<wire x1=\"7\" y1=\"8\" x2=\"9\" y2=\"9\" width=\"0.567\" layer=\"2\"/>")),
      parseagle::Wire(dom("<wire x1=\"7\" y1=\"8\" x2=\"9\" y2=\"9\" width=\"-1\" layer=\"2\"/>")),
      // clang-format on
  };
  auto out = c.convertAndJoinWires(wires, true, log);
  ASSERT_EQ(4, out.count());
  EXPECT_EQ(1, log.getMessages().count());

  const EagleTypeConverter::Geometry* geometry = &out.at(0);
  EXPECT_EQ(1, geometry->layerId);
  EXPECT_EQ(UnsignedLength(0), geometry->lineWidth);  // Converted to area
  EXPECT_EQ(true, geometry->filled);
  EXPECT_EQ(true, geometry->grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(7000000, 8283500), Angle(0)),
                Vertex(Point(9000000, 8283500), Angle(0)),
                Vertex(Point(9000000, 7716500), Angle(0)),
                Vertex(Point(7000000, 7716500), Angle(0)),
                Vertex(Point(7000000, 8283500), Angle(0)),
            }),
            geometry->path);

  geometry = &out.at(1);
  EXPECT_EQ(1, geometry->layerId);
  EXPECT_EQ(UnsignedLength(254000), geometry->lineWidth);
  EXPECT_EQ(false, geometry->filled);
  EXPECT_EQ(false, geometry->grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(1000000, 2000000), Angle(0)),
                Vertex(Point(3000000, 4000000), Angle(0)),
                Vertex(Point(5000000, 6000000), Angle(0)),
            }),
            geometry->path);

  geometry = &out.at(2);
  EXPECT_EQ(1, geometry->layerId);
  EXPECT_EQ(UnsignedLength(567000), geometry->lineWidth);
  EXPECT_EQ(false, geometry->filled);
  EXPECT_EQ(false, geometry->grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(5000000, 6000000), Angle(0)),
                Vertex(Point(7000000, 8000000), Angle(0)),
            }),
            geometry->path);

  geometry = &out.at(3);
  EXPECT_EQ(2, geometry->layerId);
  EXPECT_EQ(UnsignedLength(567000), geometry->lineWidth);
  EXPECT_EQ(false, geometry->filled);
  EXPECT_EQ(false, geometry->grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(7000000, 8000000), Angle(0)),
                Vertex(Point(9000000, 9000000), Angle(0)),
            }),
            geometry->path);
}

TEST_F(EagleTypeConverterTest, testConvertRectangle) {
  EagleTypeConverter c;
  QString xml = "<rectangle x1=\"1\" y1=\"2\" x2=\"4\" y2=\"3\" layer=\"1\"/>";
  auto out = c.convertRectangle(parseagle::Rectangle(dom(xml)), true);
  EXPECT_EQ(1, out.layerId);
  EXPECT_EQ(UnsignedLength(0), out.lineWidth);
  EXPECT_EQ(true, out.filled);  // EAGLE rectangles are always filled.
  EXPECT_EQ(true, out.grabArea);  // Passed to function under test.
  EXPECT_EQ(Path({
                Vertex(Point(1000000, 2000000), Angle(0)),
                Vertex(Point(4000000, 2000000), Angle(0)),
                Vertex(Point(4000000, 3000000), Angle(0)),
                Vertex(Point(1000000, 3000000), Angle(0)),
                Vertex(Point(1000000, 2000000), Angle(0)),
            }),
            out.path);
  EXPECT_EQ(std::nullopt, out.circle);
}

TEST_F(EagleTypeConverterTest, testConvertRectangleRotated) {
  EagleTypeConverter c;
  QString xml =
      "<rectangle x1=\"1\" y1=\"2\" x2=\"4\" y2=\"3\" layer=\"1\" "
      "rot=\"R90\"/>";
  auto out = c.convertRectangle(parseagle::Rectangle(dom(xml)), false);
  EXPECT_EQ(1, out.layerId);
  EXPECT_EQ(UnsignedLength(0), out.lineWidth);
  EXPECT_EQ(true, out.filled);  // EAGLE rectangles are always filled.
  EXPECT_EQ(false, out.grabArea);  // Passed to function under test.
  EXPECT_EQ(Path({
                Vertex(Point(3000000, 1000000), Angle(0)),
                Vertex(Point(3000000, 4000000), Angle(0)),
                Vertex(Point(2000000, 4000000), Angle(0)),
                Vertex(Point(2000000, 1000000), Angle(0)),
                Vertex(Point(3000000, 1000000), Angle(0)),
            }),
            out.path);
  EXPECT_EQ(std::nullopt, out.circle);
}

TEST_F(EagleTypeConverterTest, testConvertPolygon) {
  EagleTypeConverter c;
  QString xml =
      "<polygon width=\"2.54\" layer=\"1\">"
      "<vertex x=\"1\" y=\"2\" curve=\"45\"/>"
      "<vertex x=\"3\" y=\"4\"/>"
      "</polygon>";
  auto out = c.convertPolygon(parseagle::Polygon(dom(xml)), false);
  EXPECT_EQ(1, out.layerId);
  EXPECT_EQ(UnsignedLength(2540000), out.lineWidth);
  EXPECT_EQ(true, out.filled);  // EAGLE polygons are always filled.
  EXPECT_EQ(false, out.grabArea);  // Passed to function under test.
  EXPECT_EQ(Path({
                Vertex(Point(1000000, 2000000), Angle(45000000)),
                Vertex(Point(3000000, 4000000), Angle(0)),
                Vertex(Point(1000000, 2000000), Angle(0)),
            }),
            out.path);
  EXPECT_EQ(std::nullopt, out.circle);
}

TEST_F(EagleTypeConverterTest, testConvertCircle) {
  EagleTypeConverter c;
  QString xml =
      "<circle x=\"1\" y=\"2\" radius=\"3.5\" width=\"0.254\" layer=\"1\"/>";
  auto out = c.convertCircle(parseagle::Circle(dom(xml)), true);
  EXPECT_EQ(1, out.layerId);
  EXPECT_EQ(UnsignedLength(254000), out.lineWidth);
  EXPECT_EQ(false, out.filled);  // Not filled if line width != 0.
  EXPECT_EQ(true, out.grabArea);  // Passed to function under test.
  EXPECT_EQ(Path({
                Vertex(Point(4500000, 2000000), -Angle::deg180()),
                Vertex(Point(-2500000, 2000000), -Angle::deg180()),
                Vertex(Point(4500000, 2000000), Angle(0)),
            }),
            out.path);
  ASSERT_TRUE(out.circle.has_value());
  EXPECT_EQ(Point(1000000, 2000000), out.circle->first);
  EXPECT_EQ(PositiveLength(7000000), out.circle->second);
}

TEST_F(EagleTypeConverterTest, testConvertCircleFilled) {
  EagleTypeConverter c;
  QString xml =
      "<circle x=\"1\" y=\"2\" radius=\"3.5\" width=\"0\" layer=\"1\"/>";
  auto out = c.convertCircle(parseagle::Circle(dom(xml)), false);
  EXPECT_EQ(1, out.layerId);
  EXPECT_EQ(UnsignedLength(0), out.lineWidth);
  EXPECT_EQ(true, out.filled);  // Filled if line width == 0.
  EXPECT_EQ(false, out.grabArea);  // Passed to function under test.
  EXPECT_EQ(Path({
                Vertex(Point(4500000, 2000000), -Angle::deg180()),
                Vertex(Point(-2500000, 2000000), -Angle::deg180()),
                Vertex(Point(4500000, 2000000), Angle(0)),
            }),
            out.path);
  ASSERT_TRUE(out.circle.has_value());
  EXPECT_EQ(Point(1000000, 2000000), out.circle->first);
  EXPECT_EQ(PositiveLength(7000000), out.circle->second);
}

TEST_F(EagleTypeConverterTest, testConvertHole) {
  EagleTypeConverter c;
  QString xml = "<hole x=\"1\" y=\"2\" drill=\"3.5\"/>";
  auto out = c.convertHole(parseagle::Hole(dom(xml)));
  EXPECT_EQ(PositiveLength(3500000), out->getDiameter());
  EXPECT_EQ(1, out->getPath()->getVertices().count());
  EXPECT_EQ(Point(1000000, 2000000),
            out->getPath()->getVertices().first().getPos());
}

TEST_F(EagleTypeConverterTest, testConvertFrame) {
  EagleTypeConverter c;
  QString xml =
      "<frame x1=\"10\" y1=\"20\" x2=\"40\" y2=\"30\" columns=\"6\" rows=\"4\" "
      "layer=\"94\"/>";
  auto out = c.convertFrame(parseagle::Frame(dom(xml)));
  EXPECT_EQ(94, out.layerId);
  EXPECT_EQ(UnsignedLength(200000), out.lineWidth);
  EXPECT_EQ(false, out.filled);  // Filled frames make no sense.
  EXPECT_EQ(false, out.grabArea);  // Grab area makes no sense.
  EXPECT_EQ(Path({
                Vertex(Point(13810000, 23810000), Angle(0)),
                Vertex(Point(36190000, 23810000), Angle(0)),
                Vertex(Point(36190000, 26190000), Angle(0)),
                Vertex(Point(13810000, 26190000), Angle(0)),
                Vertex(Point(13810000, 23810000), Angle(0)),
            }),
            out.path);
  EXPECT_EQ(std::nullopt, out.circle);
}

TEST_F(EagleTypeConverterTest, testConvertTextValue) {
  EagleTypeConverter c;
  EXPECT_EQ("", c.convertTextValue("").toStdString());
  EXPECT_EQ("{{NAME}}", c.convertTextValue(">NAME").toStdString());
  EXPECT_EQ("{{VALUE}}", c.convertTextValue(">VALUE").toStdString());
  EXPECT_EQ("Some Text", c.convertTextValue("Some Text").toStdString());
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicTextSize) {
  EagleTypeConverter c;
  // Attention: The conversion factor is never exactly correct due to different
  // font layouting, but it seems to be a good value in most cases. Also it
  // makes sense to convert EAGLEs default name/value size of 1.778mm (is this
  // true?) to the LibrePCB's default name/value size of 2.5mm.
  EXPECT_EQ(PositiveLength(2500000), c.convertSchematicTextSize(1.778));
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicText) {
  EagleTypeConverter c;
  QString xml =
      "<text x=\"1\" y=\"2\" size=\"1.778\" layer=\"94\">foo\nbar</text>";
  auto out = c.tryConvertSchematicText(parseagle::Text(dom(xml)), true);
  ASSERT_TRUE(out);
  EXPECT_EQ(Layer::symbolOutlines().getId().toStdString(),
            out->getLayer().getId().toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out->getPosition());
  EXPECT_EQ(Angle(0), out->getRotation());
  EXPECT_EQ(PositiveLength(2500000), out->getHeight());  // Scaled.
  EXPECT_EQ(Alignment(HAlign::left(), VAlign::bottom()), out->getAlign());
  EXPECT_EQ("foo\nbar", out->getText().toStdString());
  EXPECT_EQ(true, out->isLocked());  // Because of the layer.
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardTextSize) {
  EagleTypeConverter c;
  // Attention: The conversion factor is never exactly correct due to different
  // font layouting, but it seems to be a good value for the vector font
  // (LibrePCB doesn't support other fonts anyway, so we don't care about them).
  EXPECT_EQ(PositiveLength(1700000), c.convertBoardTextSize(1, 2));
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardTextStrokeWidth) {
  EagleTypeConverter c;
  EXPECT_EQ(UnsignedLength(1050000), c.convertBoardTextStrokeWidth(1, 2.5, 42));
  // It seems the ratio is sometimes not defined and is thus set to 0%. In this
  // case, we fall back to a default ratio of 15%.
  EXPECT_EQ(UnsignedLength(375000), c.convertBoardTextStrokeWidth(1, 2.5, 0));
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardText) {
  EagleTypeConverter c;
  QString xml = "<text x=\"1\" y=\"2\" size=\"3\" layer=\"1\">&gt;NAME</text>";
  auto out = c.tryConvertBoardText(parseagle::Text(dom(xml)), true);
  ASSERT_TRUE(out);
  EXPECT_EQ(Layer::topCopper().getId().toStdString(),
            out->getLayer().getId().toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out->getPosition());
  EXPECT_EQ(Angle(0), out->getRotation());
  EXPECT_EQ(PositiveLength(2550000), out->getHeight());  // Scaled.
  EXPECT_EQ(UnsignedLength(240000), out->getStrokeWidth());  // Default ratio.
  EXPECT_EQ(StrokeTextSpacing(), out->getLetterSpacing());  // Hardcoded.
  EXPECT_EQ(StrokeTextSpacing(), out->getLineSpacing());  // Hardcoded.
  EXPECT_EQ(Alignment(HAlign::left(), VAlign::bottom()), out->getAlign());
  EXPECT_EQ(false, out->getMirrored());  // Default value.
  EXPECT_EQ(true, out->getAutoRotate());  // Default value.
  EXPECT_EQ("{{NAME}}", out->getText().toStdString());
  EXPECT_EQ(true, out->isLocked());  // Because of the layer.
}

TEST_F(EagleTypeConverterTest, testConvertSymbolPin) {
  EagleTypeConverter c;
  QString xml = "<pin name=\"P$1\" x=\"1\" y=\"2\" length=\"point\"/>";
  auto out = c.convertSymbolPin(parseagle::Pin(dom(xml)));
  EXPECT_EQ("1", out.pin->getName()->toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out.pin->getPosition());
  EXPECT_EQ(UnsignedLength(0), out.pin->getLength());
  EXPECT_EQ(Angle(0), out.pin->getRotation());
  EXPECT_EQ(nullptr, out.circle);
  EXPECT_EQ(nullptr, out.polygon);
}

TEST_F(EagleTypeConverterTest, testConvertSymbolPinRotated) {
  EagleTypeConverter c;
  QString xml =
      "<pin name=\"P$1\" x=\"1\" y=\"2\" length=\"middle\" rot=\"R90\"/>";
  auto out = c.convertSymbolPin(parseagle::Pin(dom(xml)));
  EXPECT_EQ("1", out.pin->getName()->toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out.pin->getPosition());
  EXPECT_EQ(UnsignedLength(5080000), out.pin->getLength());
  EXPECT_EQ(Angle(90000000), out.pin->getRotation());
  EXPECT_EQ(nullptr, out.circle);
  EXPECT_EQ(nullptr, out.polygon);
}

TEST_F(EagleTypeConverterTest, testConvertThtPad) {
  EagleTypeConverter c;
  QString xml =
      "<pad name=\"P$1\" x=\"1\" y=\"2\" drill=\"1.5\" shape=\"square\"/>";
  auto out =
      c.convertThtPad(parseagle::ThtPad(dom(xml)),
                      EagleTypeConverter::getDefaultAutoThtAnnularWidth());
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(0), out.second->getRotation());
  EXPECT_EQ(Pad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(2250000), out.second->getWidth());  // 1.5*drill
  EXPECT_EQ(PositiveLength(2250000), out.second->getHeight());  // 1.5*drill
  EXPECT_EQ(Pad::ComponentSide::Top, out.second->getComponentSide());
  ASSERT_EQ(1, out.second->getHoles().count());
  EXPECT_EQ(PositiveLength(1500000),
            out.second->getHoles().first()->getDiameter());
}

TEST_F(EagleTypeConverterTest, testConvertThtPadRotated) {
  EagleTypeConverter c;
  QString xml =
      "<pad name=\"P$1\" x=\"1\" y=\"2\" drill=\"1.5\" diameter=\"2.54\" "
      "shape=\"octagon\" rot=\"R90\"/>";
  auto out =
      c.convertThtPad(parseagle::ThtPad(dom(xml)),
                      EagleTypeConverter::getDefaultAutoThtAnnularWidth());
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(90000000), out.second->getRotation());
  EXPECT_EQ(Pad::Shape::RoundedOctagon, out.second->getShape());
  EXPECT_EQ(PositiveLength(2540000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(2540000), out.second->getHeight());
  EXPECT_EQ(Pad::ComponentSide::Top, out.second->getComponentSide());
  ASSERT_EQ(1, out.second->getHoles().count());
  EXPECT_EQ(PositiveLength(1500000),
            out.second->getHoles().first()->getDiameter());
}

TEST_F(EagleTypeConverterTest, testConvertSmtPad) {
  EagleTypeConverter c;
  QString xml =
      "<smd name=\"P$1\" x=\"1\" y=\"2\" dx=\"3\" dy=\"4\" layer=\"1\"/>";
  auto out = c.convertSmtPad(parseagle::SmtPad(dom(xml)));
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(0), out.second->getRotation());
  EXPECT_EQ(Pad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(3000000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(4000000), out.second->getHeight());
  EXPECT_EQ(Pad::ComponentSide::Top, out.second->getComponentSide());
  EXPECT_EQ(0, out.second->getHoles().count());
}

TEST_F(EagleTypeConverterTest, testConvertSmtPadRotated) {
  EagleTypeConverter c;
  QString xml =
      "<smd name=\"P$1\" x=\"1\" y=\"2\" dx=\"3\" dy=\"4\" layer=\"16\" "
      "rot=\"R90\"/>";
  auto out = c.convertSmtPad(parseagle::SmtPad(dom(xml)));
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(90000000), out.second->getRotation());
  EXPECT_EQ(Pad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(3000000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(4000000), out.second->getHeight());
  EXPECT_EQ(Pad::ComponentSide::Bottom, out.second->getComponentSide());
  EXPECT_EQ(0, out.second->getHoles().count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace eagleimport
}  // namespace librepcb
