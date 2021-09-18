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
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/import/dxfreader.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class DxfReaderTest : public ::testing::Test {
protected:
  DxfReader reader;  ///< The unit under test

  /**
   * @brief Helper to call reader.parse() with DXF content as bytearray
   */
  void parse(const QByteArray& dxf) {
    FilePath fp = FilePath::getRandomTempPath();
    FileUtils::writeFile(fp, dxf);
    reader.parse(fp);
    FileUtils::removeFile(fp);
  }

  /**
   * @brief Helper to easily compare objects as strings for easier debugging
   */
  static std::string str(const SerializableObject& obj) {
    return obj.serializeToDomElement("object").toByteArray().toStdString();
  }

  /**
   * @brief Helper to easily compare objects as strings for easier debugging
   */
  static std::string str(const DxfReader::Circle& circle) {
    SExpression s = SExpression::createList("object");
    s.appendChild(circle.position.serializeToDomElement("position"), false);
    s.appendChild("diameter", circle.diameter, false);
    return s.toByteArray().toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(DxfReaderTest, testInexistentFileThrowsRuntimeError) {
  FilePath fp = FilePath::getRandomTempPath();
  EXPECT_THROW(reader.parse(fp), RuntimeError);
}

TEST_F(DxfReaderTest, testEmptyFile) {
  parse("");
  EXPECT_EQ(0, reader.getPoints().count());
  EXPECT_EQ(0, reader.getPolygons().count());
  EXPECT_EQ(0, reader.getCircles().count());
}

TEST_F(DxfReaderTest, testPointNoUnitIsMillimeters) {
  parse(
      "0\nSECTION\n"
      "2\nENTITIES\n"
      "0\nPOINT\n"
      "10\n-4.0\n"  // X
      "20\n-5.0\n"  // Y
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(1, reader.getPoints().count());
  ASSERT_EQ(0, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Point expected(Length(-4000000), Length(-5000000));
  EXPECT_EQ(str(expected), str(reader.getPoints().first()));
}

TEST_F(DxfReaderTest, testPointUnspecifiedUnitIsMillimeters) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n0\n"  // UNIT = unspecified
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nPOINT\n"
      "10\n-4.0\n"  // X
      "20\n-5.0\n"  // Y
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(1, reader.getPoints().count());
  ASSERT_EQ(0, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Point expected(Length(-4000000), Length(-5000000));
  EXPECT_EQ(str(expected), str(reader.getPoints().first()));
}

TEST_F(DxfReaderTest, testPointMillimeters) {
  reader.setScaleFactor(2);
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n4\n"  // UNIT = millimeters
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nPOINT\n"
      "10\n-4.0\n"  // X
      "20\n-5.0\n"  // Y
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(1, reader.getPoints().count());
  ASSERT_EQ(0, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Point expected(Length(-8000000), Length(-10000000));
  EXPECT_EQ(str(expected), str(reader.getPoints().first()));
}

TEST_F(DxfReaderTest, testPointInches) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n1\n"  // UNIT = inches
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nPOINT\n"
      "10\n-4.0\n"  // X
      "20\n-5.0\n"  // Y
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(1, reader.getPoints().count());
  ASSERT_EQ(0, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Point expected(Length(-101600000), Length(-127000000));
  EXPECT_EQ(str(expected), str(reader.getPoints().first()));
}

TEST_F(DxfReaderTest, testCircle) {
  reader.setScaleFactor(2);
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nCIRCLE\n"
      "10\n4.0\n"  // CX
      "20\n5.0\n"  // CY
      "40\n8.0\n"  // RADIUS
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(0, reader.getPolygons().count());
  ASSERT_EQ(1, reader.getCircles().count());

  DxfReader::Circle expected{Point(Length(8000), Length(10000)),
                             PositiveLength(32000)};  // diameter, not radius!
  EXPECT_EQ(str(expected), str(reader.getCircles().first()));
}

TEST_F(DxfReaderTest, testLine) {
  reader.setScaleFactor(2);
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nLINE\n"
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "11\n8.0\n"  // X2
      "21\n10.0\n"  // Y2
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(8000), Length(10000)), Angle(0)),
      Vertex(Point(Length(16000), Length(20000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testArcFrom90To180Deg) {
  reader.setScaleFactor(2);
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nARC\n"
      "10\n4.0\n"  // CX
      "20\n5.0\n"  // CY
      "40\n2.0\n"  // RADIUS
      "50\n90.0\n"  // START ANGLE
      "51\n180.0\n"  // END ANGLE
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(8000), Length(14000)), Angle::deg90()),
      Vertex(Point(Length(4000), Length(10000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testArcFrom180To90Deg) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nARC\n"
      "10\n4.0\n"  // CX
      "20\n5.0\n"  // CY
      "40\n2.0\n"  // RADIUS
      "50\n180.0\n"  // START ANGLE
      "51\n90.0\n"  // END ANGLE
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(2000), Length(5000)), Angle::deg270()),
      Vertex(Point(Length(4000), Length(7000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testArcFromMinus90To90Deg) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nARC\n"
      "10\n4.0\n"  // CX
      "20\n5.0\n"  // CY
      "40\n2.0\n"  // RADIUS
      "50\n-90.0\n"  // START ANGLE
      "51\n90.0\n"  // END ANGLE
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(3000)), Angle::deg180()),
      Vertex(Point(Length(4000), Length(7000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testArcFrom90ToMinus90Deg) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nARC\n"
      "10\n4.0\n"  // CX
      "20\n5.0\n"  // CY
      "40\n2.0\n"  // RADIUS
      "50\n90.0\n"  // START ANGLE
      "51\n-90.0\n"  // END ANGLE
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(7000)), Angle::deg180()),
      Vertex(Point(Length(4000), Length(3000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testLwPolylineBulgeCcw) {
  reader.setScaleFactor(2);
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nLWPOLYLINE\n"
      "90\n2\n"  // NUMBER OF VERTICES
      "70\n0\n"  // FLAGS (0=open, 1=closed)
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "42\n1.0\n"  // BULGE
      "10\n6.0\n"  // X2
      "20\n5.0\n"  // Y2
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(8000), Length(10000)), Angle::deg180()),
      Vertex(Point(Length(12000), Length(10000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

// Polyline as used in DXF R12 format.
TEST_F(DxfReaderTest, testPolyline) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nPOLYLINE\n"
      "70\n0\n"  // FLAGS (0=open, 1=closed)
      "0\nVERTEX\n"
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "0\nVERTEX\n"
      "10\n4.0\n"  // X2
      "20\n7.0\n"  // Y2
      "42\n1.0\n"  // BULGE
      "0\nVERTEX\n"
      "10\n6.0\n"  // X3
      "20\n7.0\n"  // Y3
      "0\nVERTEX\n"
      "10\n6.0\n"  // X4
      "20\n5.0\n"  // Y4
      "0\nSEQEND\n"
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(5000)), Angle(0)),
      Vertex(Point(Length(4000), Length(7000)), Angle::deg180()),
      Vertex(Point(Length(6000), Length(7000)), Angle(0)),
      Vertex(Point(Length(6000), Length(5000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

// Polyline as used in recent DXF formats.
TEST_F(DxfReaderTest, testLwPolyline) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nLWPOLYLINE\n"
      "90\n4\n"  // NUMBER OF VERTICES
      "70\n0\n"  // FLAGS (0=open, 1=closed)
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "10\n4.0\n"  // X2
      "20\n7.0\n"  // Y2
      "42\n1.0\n"  // BULGE
      "10\n6.0\n"  // X3
      "20\n7.0\n"  // Y3
      "10\n6.0\n"  // X4
      "20\n5.0\n"  // Y4
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(5000)), Angle(0)),
      Vertex(Point(Length(4000), Length(7000)), Angle::deg180()),
      Vertex(Point(Length(6000), Length(7000)), Angle(0)),
      Vertex(Point(Length(6000), Length(5000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testLwPolylineBulgeCw) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nLWPOLYLINE\n"
      "90\n2\n"  // NUMBER OF VERTICES
      "70\n0\n"  // FLAGS (0=open, 1=closed)
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "42\n-1.0\n"  // BULGE
      "10\n6.0\n"  // X2
      "20\n5.0\n"  // Y2
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(5000)), -Angle::deg180()),
      Vertex(Point(Length(6000), Length(5000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

TEST_F(DxfReaderTest, testLwPolylineClosed) {
  parse(
      "0\nSECTION\n"
      "2\nHEADER\n"
      "9\n$INSUNITS\n"
      "70\n13\n"  // UNIT = micrometers
      "0\nENDSEC\n"
      "2\nENTITIES\n"
      "0\nLWPOLYLINE\n"
      "90\n3\n"  // NUMBER OF VERTICES
      "70\n1\n"  // FLAGS (0=open, 1=closed)
      "10\n4.0\n"  // X1
      "20\n5.0\n"  // Y1
      "10\n4.0\n"  // X2
      "20\n7.0\n"  // Y2
      "10\n6.0\n"  // X3
      "20\n7.0\n"  // Y3
      "0\nENDSEC\n"
      "0\nEOF\n");

  // Assert(!) for number of elements to avoid illegal list item access below.
  ASSERT_EQ(0, reader.getPoints().count());
  ASSERT_EQ(1, reader.getPolygons().count());
  ASSERT_EQ(0, reader.getCircles().count());

  Path expected({
      Vertex(Point(Length(4000), Length(5000)), Angle(0)),
      Vertex(Point(Length(4000), Length(7000)), Angle(0)),
      Vertex(Point(Length(6000), Length(7000)), Angle(0)),
      Vertex(Point(Length(4000), Length(5000)), Angle(0)),
  });
  EXPECT_EQ(str(expected), str(reader.getPolygons().first()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
