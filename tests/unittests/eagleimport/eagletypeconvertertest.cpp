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

using C = EagleTypeConverter;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class EagleTypeConverterTest : public ::testing::Test {
protected:
  static parseagle::DomElement dom(const QString& str) {
    QDomDocument doc;
    doc.setContent(str);
    return parseagle::DomElement(doc.documentElement());
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EagleTypeConverterTest, testConvertElementName) {
  EXPECT_EQ("Valid Name", C::convertElementName("Valid Name")->toStdString());
  EXPECT_EQ("X", C::convertElementName(" \nX ")->toStdString());
  EXPECT_EQ("Unnamed", C::convertElementName("\n")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertElementDescription) {
  EXPECT_EQ("", C::convertElementDescription("").toStdString());
  EXPECT_EQ("Text", C::convertElementDescription(" Text ").toStdString());
  EXPECT_EQ("X\nY", C::convertElementDescription("X\nY").toStdString());
  EXPECT_EQ("X\nY",
            C::convertElementDescription("<b>X</b><br/>Y").toStdString());
  EXPECT_EQ("X\nY",
            C::convertElementDescription("<b>X</b>\n<br/>Y").toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertComponentName) {
  EXPECT_EQ("Valid Name", C::convertComponentName("Valid Name")->toStdString());
  EXPECT_EQ("X", C::convertComponentName(" \nX ")->toStdString());
  EXPECT_EQ("Foo - Bar", C::convertComponentName("Foo - Bar-")->toStdString());
  EXPECT_EQ("Foo _ Bar", C::convertComponentName("Foo _ Bar_")->toStdString());
  EXPECT_EQ("-", C::convertComponentName("-")->toStdString());
  EXPECT_EQ("Unnamed", C::convertComponentName("\n")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertDeviceName) {
  EXPECT_EQ("Valid Name",
            C::convertDeviceName("Valid Name", "")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            C::convertDeviceName("Valid Name", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            C::convertDeviceName("Valid Name-", "Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            C::convertDeviceName("Valid Name_", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            C::convertDeviceName("Valid Name-", "Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            C::convertDeviceName("Valid Name_", "Foo")->toStdString());
  EXPECT_EQ("Valid Name-Foo",
            C::convertDeviceName("Valid Name", "-Foo")->toStdString());
  EXPECT_EQ("Valid Name_Foo",
            C::convertDeviceName("Valid Name", "_Foo")->toStdString());
  EXPECT_EQ("X", C::convertDeviceName(" \nX ", "")->toStdString());
  EXPECT_EQ("Unnamed", C::convertDeviceName("\n", "")->toStdString());
  EXPECT_EQ("Unnamed", C::convertDeviceName("", "")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertComponentPrefix) {
  EXPECT_EQ("", C::convertComponentPrefix("")->toStdString());
  EXPECT_EQ("", C::convertComponentPrefix("$42+")->toStdString());
  EXPECT_EQ("C", C::convertComponentPrefix("C")->toStdString());
  EXPECT_EQ("Foo_Bar", C::convertComponentPrefix(" Foo Bar ")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertGateName) {
  EXPECT_EQ("", C::convertGateName("")->toStdString());
  EXPECT_EQ("G42", C::convertGateName("G$42")->toStdString());
  EXPECT_EQ("1", C::convertGateName("-1")->toStdString());
  EXPECT_EQ("Foo_Bar", C::convertGateName(" Foo Bar ")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertPinOrPadName) {
  EXPECT_EQ("Unnamed", C::convertPinOrPadName(" ")->toStdString());
  EXPECT_EQ("42", C::convertPinOrPadName("P$42")->toStdString());
  EXPECT_EQ("3", C::convertPinOrPadName("3")->toStdString());
  EXPECT_EQ("Foo_Bar", C::convertPinOrPadName(" Foo Bar ")->toStdString());
  EXPECT_EQ("!FOO!/BAR", C::convertPinOrPadName("!FOO/BAR")->toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertInversionSyntax) {
  EXPECT_EQ("FOO", C::convertInversionSyntax("FOO").toStdString());
  EXPECT_EQ("!FOO", C::convertInversionSyntax("!FOO").toStdString());
  EXPECT_EQ("!FOO", C::convertInversionSyntax("!FOO!").toStdString());
  EXPECT_EQ("!FOO/BAR", C::convertInversionSyntax("!FOO!/BAR").toStdString());
  EXPECT_EQ("!FOO!/BAR", C::convertInversionSyntax("!FOO/BAR").toStdString());
  EXPECT_EQ("FOO/!BAR", C::convertInversionSyntax("FOO/!BAR").toStdString());
  EXPECT_EQ("FOO/!BAR", C::convertInversionSyntax("FOO/!BAR!").toStdString());
  EXPECT_EQ("A/!B/C", C::convertInversionSyntax("A/!B!/C").toStdString());
}

TEST_F(EagleTypeConverterTest, testConvertAttributeValid) {
  MessageLogger log;
  const QString xml = "<attribute name=\"Foo Bar\" value=\"hello world!\"/>";
  auto out = C::tryConvertAttribute(parseagle::Attribute(dom(xml)), log);
  ASSERT_TRUE(out != nullptr);
  EXPECT_EQ("FOO_BAR", out->getKey()->toStdString());
  EXPECT_EQ("hello world!", out->getValue().toStdString());
  EXPECT_EQ(AttributeType::Type_t::String, out->getType().getType());
  EXPECT_EQ(0, log.getMessages().count());
}

TEST_F(EagleTypeConverterTest, testConvertAttributeInvalid) {
  MessageLogger log;
  const QString xml = "<attribute name=\"!\" value=\"hello world!\"/>";
  auto out = C::tryConvertAttribute(parseagle::Attribute(dom(xml)), log);
  EXPECT_TRUE(out == nullptr);
  EXPECT_EQ(1, log.getMessages().count());
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicLayer) {
  EXPECT_EQ(nullptr, C::tryConvertSchematicLayer(1));  // tCu
  EXPECT_EQ(&Layer::symbolOutlines(), C::tryConvertSchematicLayer(94));  // sym
  EXPECT_EQ(nullptr, C::tryConvertSchematicLayer(999));  // non existent
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardLayer) {
  EXPECT_EQ(&Layer::topCopper(), C::tryConvertBoardLayer(1));  // tCu
  EXPECT_EQ(Layer::innerCopper(2), C::tryConvertBoardLayer(3));  // inner 2
  EXPECT_EQ(&Layer::botCopper(), C::tryConvertBoardLayer(16));  // bCu
  EXPECT_EQ(nullptr, C::tryConvertBoardLayer(94));  // symbols
  EXPECT_EQ(nullptr, C::tryConvertBoardLayer(999));  // non existent
}

TEST_F(EagleTypeConverterTest, testConvertLayerSetup) {
  typedef QHash<const Layer*, const Layer*> T;
  EXPECT_EQ((T{}), C::convertLayerSetup(""));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            C::convertLayerSetup("1*16"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            C::convertLayerSetup("(1*16)"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {Layer::innerCopper(1), Layer::innerCopper(1)},
                {Layer::innerCopper(2), Layer::innerCopper(2)},
                {Layer::innerCopper(13), Layer::innerCopper(3)},
                {Layer::innerCopper(14), Layer::innerCopper(4)},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            C::convertLayerSetup("[2:1+((2*3)+(14*15))+16:15]"));
  EXPECT_EQ((T{
                {&Layer::topCopper(), &Layer::topCopper()},
                {Layer::innerCopper(1), Layer::innerCopper(1)},
                {Layer::innerCopper(2), Layer::innerCopper(2)},
                {Layer::innerCopper(3), Layer::innerCopper(3)},
                {Layer::innerCopper(4), Layer::innerCopper(4)},
                {&Layer::botCopper(), &Layer::botCopper()},
            }),
            C::convertLayerSetup("[2:1+[3:2+(3*4)+5:4]+16:5]"));
  EXPECT_THROW(C::convertLayerSetup("1*Foo*16"), Exception);
}

TEST_F(EagleTypeConverterTest, testConvertAlignment) {
  EXPECT_EQ(Qt::AlignBottom | Qt::AlignRight,
            C::convertAlignment(parseagle::Alignment::BottomRight).toQtAlign());
  EXPECT_EQ(Qt::AlignTop | Qt::AlignHCenter,
            C::convertAlignment(parseagle::Alignment::TopCenter).toQtAlign());
}

TEST_F(EagleTypeConverterTest, testConvertLength) {
  EXPECT_EQ(Length(0), C::convertLength(0));
  EXPECT_EQ(Length(-1234567), C::convertLength(-1.234567));
  EXPECT_EQ(Length(1234567), C::convertLength(1.234567));
}

TEST_F(EagleTypeConverterTest, testConvertLineWidth) {
  EXPECT_EQ(UnsignedLength(0), C::convertLineWidth(0, 20));  // dimension
  EXPECT_EQ(UnsignedLength(0), C::convertLineWidth(0, 46));  // milling
  EXPECT_EQ(UnsignedLength(0), C::convertLineWidth(1.23, 20));  // dimension
  EXPECT_EQ(UnsignedLength(0), C::convertLineWidth(1.23, 46));  // milling
  EXPECT_EQ(UnsignedLength(1230000), C::convertLineWidth(1.23, 1));  // tCu
  EXPECT_EQ(UnsignedLength(1230000), C::convertLineWidth(1.23, 94));  // symbols
  EXPECT_THROW(C::convertLineWidth(-1.23, 94), Exception);
}

TEST_F(EagleTypeConverterTest, testConvertPoint) {
  EXPECT_EQ(Point(0, 0), C::convertPoint(parseagle::Point{0, 0}));
  EXPECT_EQ(Point(-1234567, 1234567),
            C::convertPoint(parseagle::Point{-1.234567, 1.234567}));
}

TEST_F(EagleTypeConverterTest, testConvertAngle) {
  EXPECT_EQ(Angle(0), C::convertAngle(0));
  EXPECT_EQ(Angle(-1234567), C::convertAngle(-1.234567));
  EXPECT_EQ(Angle(1234567), C::convertAngle(1.234567));
}

TEST_F(EagleTypeConverterTest, testConvertVertex) {
  EXPECT_EQ(
      Vertex(Point(0, 0), Angle(0)),
      C::convertVertex(parseagle::Vertex(dom("<vertex x=\"0\" y=\"0\"/>"))));
  EXPECT_EQ(Vertex(Point(-6350000, 2540000), Angle(90000000)),
            C::convertVertex(parseagle::Vertex(
                dom("<vertex x=\"-6.35\" y=\"2.54\" curve=\"90\"/>"))));
}

TEST_F(EagleTypeConverterTest, testConvertVertices) {
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
  EXPECT_EQ(expected, C::convertVertices(vertices, true));
}

TEST_F(EagleTypeConverterTest, testConvertAndJoinWires) {
  MessageLogger log;
  QList<parseagle::Wire> wires{
      // clang-format off
      parseagle::Wire(dom("<wire x1=\"1\" y1=\"2\" x2=\"3\" y2=\"4\" width=\"0.254\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"3\" y1=\"4\" x2=\"5\" y2=\"6\" width=\"0.254\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"5\" y1=\"6\" x2=\"7\" y2=\"8\" width=\"0.567\" layer=\"1\"/>")),
      parseagle::Wire(dom("<wire x1=\"7\" y1=\"8\" x2=\"9\" y2=\"9\" width=\"0.567\" layer=\"2\"/>")),
      parseagle::Wire(dom("<wire x1=\"7\" y1=\"8\" x2=\"9\" y2=\"9\" width=\"-1\" layer=\"2\"/>")),
      // clang-format on
  };
  auto out = C::convertAndJoinWires(wires, true, log);
  ASSERT_EQ(3, out.count());
  EXPECT_EQ(1, log.getMessages().count());

  EXPECT_EQ(1, out.at(0).layerId);
  EXPECT_EQ(UnsignedLength(254000), out.at(0).lineWidth);
  EXPECT_EQ(false, out.at(0).filled);
  EXPECT_EQ(false, out.at(0).grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(1000000, 2000000), Angle(0)),
                Vertex(Point(3000000, 4000000), Angle(0)),
                Vertex(Point(5000000, 6000000), Angle(0)),
            }),
            out.at(0).path);

  EXPECT_EQ(1, out.at(1).layerId);
  EXPECT_EQ(UnsignedLength(567000), out.at(1).lineWidth);
  EXPECT_EQ(false, out.at(1).filled);
  EXPECT_EQ(false, out.at(1).grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(5000000, 6000000), Angle(0)),
                Vertex(Point(7000000, 8000000), Angle(0)),
            }),
            out.at(1).path);

  EXPECT_EQ(2, out.at(2).layerId);
  EXPECT_EQ(UnsignedLength(567000), out.at(2).lineWidth);
  EXPECT_EQ(false, out.at(2).filled);
  EXPECT_EQ(false, out.at(2).grabArea);
  EXPECT_EQ(Path({
                Vertex(Point(7000000, 8000000), Angle(0)),
                Vertex(Point(9000000, 9000000), Angle(0)),
            }),
            out.at(2).path);
}

TEST_F(EagleTypeConverterTest, testConvertRectangle) {
  QString xml = "<rectangle x1=\"1\" y1=\"2\" x2=\"4\" y2=\"3\" layer=\"1\"/>";
  auto out = C::convertRectangle(parseagle::Rectangle(dom(xml)), true);
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
  QString xml =
      "<rectangle x1=\"1\" y1=\"2\" x2=\"4\" y2=\"3\" layer=\"1\" "
      "rot=\"R90\"/>";
  auto out = C::convertRectangle(parseagle::Rectangle(dom(xml)), false);
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
  QString xml =
      "<polygon width=\"2.54\" layer=\"1\">"
      "<vertex x=\"1\" y=\"2\" curve=\"45\"/>"
      "<vertex x=\"3\" y=\"4\"/>"
      "</polygon>";
  auto out = C::convertPolygon(parseagle::Polygon(dom(xml)), false);
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
  QString xml =
      "<circle x=\"1\" y=\"2\" radius=\"3.5\" width=\"0.254\" layer=\"1\"/>";
  auto out = C::convertCircle(parseagle::Circle(dom(xml)), true);
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
  QString xml =
      "<circle x=\"1\" y=\"2\" radius=\"3.5\" width=\"0\" layer=\"1\"/>";
  auto out = C::convertCircle(parseagle::Circle(dom(xml)), false);
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
  QString xml = "<hole x=\"1\" y=\"2\" drill=\"3.5\"/>";
  auto out = C::convertHole(parseagle::Hole(dom(xml)));
  EXPECT_EQ(PositiveLength(3500000), out->getDiameter());
  EXPECT_EQ(1, out->getPath()->getVertices().count());
  EXPECT_EQ(Point(1000000, 2000000),
            out->getPath()->getVertices().first().getPos());
}

TEST_F(EagleTypeConverterTest, testConvertFrame) {
  QString xml =
      "<frame x1=\"10\" y1=\"20\" x2=\"40\" y2=\"30\" columns=\"6\" rows=\"4\" "
      "layer=\"94\"/>";
  auto out = C::convertFrame(parseagle::Frame(dom(xml)));
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
  EXPECT_EQ("", C::convertTextValue("").toStdString());
  EXPECT_EQ("{{NAME}}", C::convertTextValue(">NAME").toStdString());
  EXPECT_EQ("{{VALUE}}", C::convertTextValue(">VALUE").toStdString());
  EXPECT_EQ("Some Text", C::convertTextValue("Some Text").toStdString());
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicTextSize) {
  // Attention: The conversion factor is never exactly correct due to different
  // font layouting, but it seems to be a good value in most cases. Also it
  // makes sense to convert EAGLEs default name/value size of 1.778mm (is this
  // true?) to the LibrePCB's default name/value size of 2.5mm.
  EXPECT_EQ(PositiveLength(2500000), C::convertSchematicTextSize(1.778));
}

TEST_F(EagleTypeConverterTest, testTryConvertSchematicText) {
  QString xml =
      "<text x=\"1\" y=\"2\" size=\"1.778\" layer=\"94\">foo\nbar</text>";
  auto out = C::tryConvertSchematicText(parseagle::Text(dom(xml)), true);
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
  // Attention: The conversion factor is never exactly correct due to different
  // font layouting, but it seems to be a good value for the vector font
  // (LibrePCB doesn't support other fonts anyway, so we don't care about them).
  EXPECT_EQ(PositiveLength(1700000), C::convertBoardTextSize(1, 2));
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardTextStrokeWidth) {
  EXPECT_EQ(UnsignedLength(1050000),
            C::convertBoardTextStrokeWidth(1, 2.5, 42));
  // It seems the ratio is sometimes not defined and is thus set to 0%. In this
  // case, we fall back to a default ratio of 15%.
  EXPECT_EQ(UnsignedLength(375000), C::convertBoardTextStrokeWidth(1, 2.5, 0));
}

TEST_F(EagleTypeConverterTest, testTryConvertBoardText) {
  QString xml = "<text x=\"1\" y=\"2\" size=\"3\" layer=\"1\">&gt;NAME</text>";
  auto out = C::tryConvertBoardText(parseagle::Text(dom(xml)));
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
}

TEST_F(EagleTypeConverterTest, testConvertSymbolPin) {
  QString xml = "<pin name=\"P$1\" x=\"1\" y=\"2\" length=\"point\"/>";
  auto out = C::convertSymbolPin(parseagle::Pin(dom(xml)));
  EXPECT_EQ("1", out.pin->getName()->toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out.pin->getPosition());
  EXPECT_EQ(UnsignedLength(0), out.pin->getLength());
  EXPECT_EQ(Angle(0), out.pin->getRotation());
  EXPECT_EQ(nullptr, out.circle);
  EXPECT_EQ(nullptr, out.polygon);
}

TEST_F(EagleTypeConverterTest, testConvertSymbolPinRotated) {
  QString xml =
      "<pin name=\"P$1\" x=\"1\" y=\"2\" length=\"middle\" rot=\"R90\"/>";
  auto out = C::convertSymbolPin(parseagle::Pin(dom(xml)));
  EXPECT_EQ("1", out.pin->getName()->toStdString());
  EXPECT_EQ(Point(1000000, 2000000), out.pin->getPosition());
  EXPECT_EQ(UnsignedLength(5080000), out.pin->getLength());
  EXPECT_EQ(Angle(90000000), out.pin->getRotation());
  EXPECT_EQ(nullptr, out.circle);
  EXPECT_EQ(nullptr, out.polygon);
}

TEST_F(EagleTypeConverterTest, testConvertThtPad) {
  QString xml =
      "<pad name=\"P$1\" x=\"1\" y=\"2\" drill=\"1.5\" shape=\"square\"/>";
  auto out = C::convertThtPad(parseagle::ThtPad(dom(xml)),
                              C::getDefaultAutoThtAnnularWidth());
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(0), out.second->getRotation());
  EXPECT_EQ(FootprintPad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(2250000), out.second->getWidth());  // 1.5*drill
  EXPECT_EQ(PositiveLength(2250000), out.second->getHeight());  // 1.5*drill
  EXPECT_EQ(FootprintPad::ComponentSide::Top, out.second->getComponentSide());
  ASSERT_EQ(1, out.second->getHoles().count());
  EXPECT_EQ(PositiveLength(1500000),
            out.second->getHoles().first()->getDiameter());
}

TEST_F(EagleTypeConverterTest, testConvertThtPadRotated) {
  QString xml =
      "<pad name=\"P$1\" x=\"1\" y=\"2\" drill=\"1.5\" diameter=\"2.54\" "
      "shape=\"octagon\" rot=\"R90\"/>";
  auto out = C::convertThtPad(parseagle::ThtPad(dom(xml)),
                              C::getDefaultAutoThtAnnularWidth());
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(90000000), out.second->getRotation());
  EXPECT_EQ(FootprintPad::Shape::RoundedOctagon, out.second->getShape());
  EXPECT_EQ(PositiveLength(2540000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(2540000), out.second->getHeight());
  EXPECT_EQ(FootprintPad::ComponentSide::Top, out.second->getComponentSide());
  ASSERT_EQ(1, out.second->getHoles().count());
  EXPECT_EQ(PositiveLength(1500000),
            out.second->getHoles().first()->getDiameter());
}

TEST_F(EagleTypeConverterTest, testConvertSmtPad) {
  QString xml =
      "<smd name=\"P$1\" x=\"1\" y=\"2\" dx=\"3\" dy=\"4\" layer=\"1\"/>";
  auto out = C::convertSmtPad(parseagle::SmtPad(dom(xml)));
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(0), out.second->getRotation());
  EXPECT_EQ(FootprintPad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(3000000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(4000000), out.second->getHeight());
  EXPECT_EQ(FootprintPad::ComponentSide::Top, out.second->getComponentSide());
  EXPECT_EQ(0, out.second->getHoles().count());
}

TEST_F(EagleTypeConverterTest, testConvertSmtPadRotated) {
  QString xml =
      "<smd name=\"P$1\" x=\"1\" y=\"2\" dx=\"3\" dy=\"4\" layer=\"16\" "
      "rot=\"R90\"/>";
  auto out = C::convertSmtPad(parseagle::SmtPad(dom(xml)));
  EXPECT_EQ("1", out.first->getName()->toStdString());
  EXPECT_EQ(out.first->getUuid(), out.second->getPackagePadUuid());
  EXPECT_EQ(Point(1000000, 2000000), out.second->getPosition());
  EXPECT_EQ(Angle(90000000), out.second->getRotation());
  EXPECT_EQ(FootprintPad::Shape::RoundedRect, out.second->getShape());
  EXPECT_EQ(PositiveLength(3000000), out.second->getWidth());
  EXPECT_EQ(PositiveLength(4000000), out.second->getHeight());
  EXPECT_EQ(FootprintPad::ComponentSide::Bottom,
            out.second->getComponentSide());
  EXPECT_EQ(0, out.second->getHoles().count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace eagleimport
}  // namespace librepcb
