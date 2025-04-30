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
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/serialization/sexpression.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class PathTest : public ::testing::Test {
protected:
  std::string str(const Path& path) const {
    std::unique_ptr<SExpression> sexpr = SExpression::createList("path");
    path.serialize(*sexpr);
    return sexpr->toByteArray().toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(PathTest, testDefaultConstructorCreatesEmptyPath) {
  Path path;
  EXPECT_EQ(0, path.getVertices().count());
}

TEST_F(PathTest, testIsCurvedFalse) {
  EXPECT_FALSE(Path().isCurved());
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0)),
                    })
                   .isCurved());
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0)),
                        Vertex(Point(1, 1)),
                    })
                   .isCurved());
}

// Ensure that the angle of the last vertex is not relevant.
TEST_F(PathTest, testIsCurvedLastVertexFalse) {
  EXPECT_FALSE(Path({
                        Vertex(Point(1, 1), Angle::deg90()),
                    })
                   .isCurved());
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0)),
                        Vertex(Point(1, 1), Angle::deg90()),
                    })
                   .isCurved());
}

TEST_F(PathTest, testIsCurvedTrue) {
  EXPECT_TRUE(Path({
                       Vertex(Point(0, 0), Angle::deg90()),
                       Vertex(Point(1, 1)),
                   })
                  .isCurved());
}

TEST_F(PathTest, testIsZeroLengthTrue) {
  EXPECT_TRUE(Path().isZeroLength());
  EXPECT_TRUE(Path({Vertex(Point(0, 0), Angle::deg90())}).isZeroLength());
  EXPECT_TRUE(Path({
                       Vertex(Point(0, 0), Angle::deg90()),
                       Vertex(Point(0, 0)),
                   })
                  .isZeroLength());
  EXPECT_TRUE(Path({
                       Vertex(Point(0, 0)),
                       Vertex(Point(0, 0)),
                       Vertex(Point(0, 0)),
                   })
                  .isZeroLength());
}

TEST_F(PathTest, testIsZeroLengthFalse) {
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0)),
                        Vertex(Point(0, 1)),
                    })
                   .isZeroLength());
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0)),
                        Vertex(Point(0, 0)),
                        Vertex(Point(1, 0)),
                    })
                   .isZeroLength());
}

TEST_F(PathTest, testIsOnGrid) {
  const PositiveLength grid(1000000);
  EXPECT_TRUE(Path().isOnGrid(grid));  // Usually this is the good case.
  EXPECT_TRUE(Path({
                       Vertex(Point(0, 0), Angle::deg90()),
                       Vertex(Point(1000000, 2000000)),
                   })
                  .isOnGrid(grid));
  EXPECT_FALSE(Path({
                        Vertex(Point(1, 0), Angle::deg90()),
                        Vertex(Point(1000000, 2000000)),
                    })
                   .isOnGrid(grid));
  EXPECT_FALSE(Path({
                        Vertex(Point(0, 0), Angle::deg90()),
                        Vertex(Point(1000000, 2000001)),
                    })
                   .isOnGrid(grid));
}

TEST_F(PathTest, testGetTotalStraightLength) {
  QVector<Vertex> vertices;
  EXPECT_EQ(UnsignedLength(0), Path(vertices).getTotalStraightLength());
  vertices.append(Vertex(Point(10, 0)));
  EXPECT_EQ(UnsignedLength(0), Path(vertices).getTotalStraightLength());
  vertices.append(Vertex(Point(10, 10)));
  EXPECT_EQ(UnsignedLength(10), Path(vertices).getTotalStraightLength());
  vertices.append(Vertex(Point(10, 0)));
  EXPECT_EQ(UnsignedLength(20), Path(vertices).getTotalStraightLength());
}

TEST_F(PathTest, testToSvgPathEmpty) {
  const Path input = Path();
  const QString expected = "";
  const QString actual = input.toSvgPathMm();
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST_F(PathTest, testToSvgPathOneVertex) {
  const Path input = Path({Vertex(Point(1000000, 1234567), Angle::deg45())});
  const QString expected = "M 1 -1.234567";
  const QString actual = input.toSvgPathMm();
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST_F(PathTest, testToSvgPath) {
  const Path input = Path({
      Vertex(Point(1000000, 1234567), Angle::deg0()),
      Vertex(Point(0, 0), Angle::deg0()),
      Vertex(Point(1000000, 1234567), Angle::deg0()),
  });
  const QString expected = "M 1 -1.234567 L 0 0 L 1 -1.234567";
  const QString actual = input.toSvgPathMm();
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST_F(PathTest, testReverseEmptyPath) {
  Path input = Path();
  Path expected = Path();
  Path actual = input.reverse();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testReverseOneVertex) {
  Path input = Path({Vertex(Point(1, 2), Angle::deg90())});
  Path expected = Path({Vertex(Point(1, 2))});
  Path actual = input.reverse();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testReverseMultipleVertices) {
  const Path input = Path({
      Vertex(Point(1, 2), Angle::deg90()),
      Vertex(Point(3, 4), Angle::deg180()),
      Vertex(Point(5, 6), Angle::deg270()),
      Vertex(Point(7, 8)),
  });
  Path expected = Path({
      Vertex(Point(7, 8), -Angle::deg270()),
      Vertex(Point(5, 6), -Angle::deg180()),
      Vertex(Point(3, 4), -Angle::deg90()),
      Vertex(Point(1, 2)),
  });
  Path actual = Path(input).reverse();
  EXPECT_EQ(str(expected), str(actual));

  // Sanity check that reversing again restores the original path.
  actual.reverse();
  EXPECT_EQ(str(input), str(actual));
}

TEST_F(PathTest, testReversed) {
  const Path input = Path({
      Vertex(Point(1, 2), Angle::deg90()),
      Vertex(Point(3, 4), Angle::deg180()),
      Vertex(Point(5, 6), Angle::deg270()),
      Vertex(Point(7, 8)),
  });
  Path expected = Path({
      Vertex(Point(7, 8), -Angle::deg270()),
      Vertex(Point(5, 6), -Angle::deg180()),
      Vertex(Point(3, 4), -Angle::deg90()),
      Vertex(Point(1, 2)),
  });
  Path actual = input.reversed();
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(PathTest, testFlattenArcsEmptyPath) {
  Path input = Path();
  const Path expected = Path();
  const Path actual = input.flattenArcs(PositiveLength(1));
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testFlattenArcsOneVertex) {
  Path input = Path({Vertex(Point(10, 20), Angle::deg180())});
  const Path expected = Path({Vertex(Point(10, 20), Angle::deg0())});
  const Path actual = input.flattenArcs(PositiveLength(1));
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testFlattenArcsTwoVerticesArc) {
  Path input = Path({
      Vertex(Point(1000, 2000), Angle::deg180()),
      Vertex(Point(1000, 3000), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1000, 2000), Angle::deg0()),
      Vertex(Point(1433, 2250), Angle::deg0()),
      Vertex(Point(1433, 2750), Angle::deg0()),
      Vertex(Point(1000, 3000), Angle::deg0()),
  });
  const Path actual = input.flattenArcs(PositiveLength(600));
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testFlattenArcsMultipleVertices) {
  Path input = Path({
      Vertex(Point(1000, 1000), Angle::deg180()),
      Vertex(Point(1000, 2000), Angle::deg0()),
      Vertex(Point(1000, 3000), Angle::deg180()),
      Vertex(Point(1000, 4000), Angle::deg0()),
      Vertex(Point(1000, 5000), Angle::deg180()),
      Vertex(Point(1000, 6000), Angle::deg180()),
      Vertex(Point(1000, 7000), Angle::deg180()),
      Vertex(Point(1000, 8000), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1000, 1000), Angle::deg0()),
      Vertex(Point(1433, 1250), Angle::deg0()),
      Vertex(Point(1433, 1750), Angle::deg0()),
      Vertex(Point(1000, 2000), Angle::deg0()),
      Vertex(Point(1000, 3000), Angle::deg0()),
      Vertex(Point(1433, 3250), Angle::deg0()),
      Vertex(Point(1433, 3750), Angle::deg0()),
      Vertex(Point(1000, 4000), Angle::deg0()),
      Vertex(Point(1000, 5000), Angle::deg0()),
      Vertex(Point(1433, 5250), Angle::deg0()),
      Vertex(Point(1433, 5750), Angle::deg0()),
      Vertex(Point(1000, 6000), Angle::deg0()),
      Vertex(Point(1433, 6250), Angle::deg0()),
      Vertex(Point(1433, 6750), Angle::deg0()),
      Vertex(Point(1000, 7000), Angle::deg0()),
      Vertex(Point(1433, 7250), Angle::deg0()),
      Vertex(Point(1433, 7750), Angle::deg0()),
      Vertex(Point(1000, 8000), Angle::deg0()),

  });
  const Path actual = input.flattenArcs(PositiveLength(600));
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_EQ(str(expected), str(input));
}

TEST_F(PathTest, testFlattenedArcs) {
  const Path input = Path({
      Vertex(Point(1000, 2000), Angle::deg180()),
      Vertex(Point(1000, 3000), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1000, 2000), Angle::deg0()),
      Vertex(Point(1433, 2250), Angle::deg0()),
      Vertex(Point(1433, 2750), Angle::deg0()),
      Vertex(Point(1000, 3000), Angle::deg0()),
  });
  const Path actual = input.flattenedArcs(PositiveLength(600));
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(PathTest, testCleanEmptyPath) {
  Path actual = Path();
  const Path expected = Path();
  const bool modified = actual.clean();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_FALSE(modified);
}

TEST_F(PathTest, testCleanOneVertex) {
  Path actual = Path({Vertex(Point(1, 2), Angle::deg90())});
  const Path expected = Path({Vertex(Point(1, 2), Angle::deg90())});
  const bool modified = actual.clean();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_FALSE(modified);
}

TEST_F(PathTest, testCleanMultipleVertices) {
  Path actual = Path({
      Vertex(Point(1, 2), Angle::deg45()),
      Vertex(Point(1, 2), Angle::deg90()),  // duplicate
      Vertex(Point(3, 4), Angle::deg0()), Vertex(Point(5, 6), Angle::deg0()),
      Vertex(Point(5, 6), Angle::deg180()),  // duplicate
      Vertex(Point(5, 6), Angle::deg270()),  // duplicate
      Vertex(Point(7, 8), Angle::deg0()), Vertex(Point(9, 9), Angle::deg180()),
      Vertex(Point(9, 9), Angle::deg270()),  // duplicate
  });
  const Path expected = Path({
      Vertex(Point(1, 2), Angle::deg90()),
      Vertex(Point(3, 4), Angle::deg0()),
      Vertex(Point(5, 6), Angle::deg270()),
      Vertex(Point(7, 8), Angle::deg0()),
      Vertex(Point(9, 9), Angle::deg270()),
  });
  const bool modified = actual.clean();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(modified);
}

TEST_F(PathTest, testOpenEmptyPath) {
  Path actual = Path();
  const Path expected = Path();
  const bool modified = actual.open();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_FALSE(modified);
}

TEST_F(PathTest, testOpenTwoVertices) {
  Path actual = Path({
      Vertex(Point(1, 2), Angle::deg180()),
      Vertex(Point(1, 2), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1, 2), Angle::deg180()),
      Vertex(Point(1, 2), Angle::deg180()),
  });
  const bool modified = actual.open();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_FALSE(modified);
}

TEST_F(PathTest, testOpenMultipleVerticesClosed) {
  Path actual = Path({
      Vertex(Point(1, 2), Angle::deg45()),
      Vertex(Point(3, 4), Angle::deg90()),
      Vertex(Point(1, 2), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1, 2), Angle::deg45()),
      Vertex(Point(3, 4), Angle::deg90()),
  });
  const bool modified = actual.open();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(modified);
}

TEST_F(PathTest, testOpenMultipleVerticesOpen) {
  Path actual = Path({
      Vertex(Point(1, 2), Angle::deg45()),
      Vertex(Point(3, 4), Angle::deg90()),
      Vertex(Point(5, 6), Angle::deg180()),
  });
  const Path expected = Path({
      Vertex(Point(1, 2), Angle::deg45()),
      Vertex(Point(3, 4), Angle::deg90()),
      Vertex(Point(5, 6), Angle::deg180()),
  });
  const bool modified = actual.open();
  EXPECT_EQ(str(expected), str(actual));
  EXPECT_FALSE(modified);
}

TEST_F(PathTest, testOperatorCompareLess) {
  EXPECT_FALSE(Path() < Path());
  EXPECT_FALSE(Path({Vertex(Point(1, 2))}) < Path());
  EXPECT_FALSE(Path({Vertex(Point(1, 2))}) < Path({Vertex(Point(1, 2))}));
  EXPECT_FALSE(Path({Vertex(Point(2, 2))}) < Path({Vertex(Point(1, 2))}));
  EXPECT_FALSE(Path({Vertex(Point(0, 0), Angle::deg90())}) <
               Path({Vertex(Point(0, 0), Angle::deg0())}));

  EXPECT_TRUE(Path() < Path({Vertex(Point(1, 2))}));
  EXPECT_TRUE(Path({Vertex(Point(1, 2))}) < Path({Vertex(Point(2, 2))}));
  EXPECT_TRUE(Path({Vertex(Point(0, 0), Angle::deg0())}) <
              Path({Vertex(Point(0, 0), Angle::deg90())}));
}

TEST_F(PathTest, testLine) {
  Point p1(Length(12), Length(34));
  Point p2(Length(56), Length(78));
  Angle angle(1234);
  Path path = Path::line(p1, p2, angle);
  EXPECT_EQ(2, path.getVertices().count());
  EXPECT_EQ(p1, path.getVertices().value(0).getPos());
  EXPECT_EQ(angle, path.getVertices().value(0).getAngle());
  EXPECT_EQ(p2, path.getVertices().value(1).getPos());
  EXPECT_EQ(Angle(0), path.getVertices().value(1).getAngle());
  EXPECT_FALSE(path.isClosed());
}

TEST_F(PathTest, testCircle) {
  PositiveLength diameter(1000);
  Path path = Path::circle(diameter);
  Point p1(Length(500), Length(0));
  Point p2(Length(-500), Length(0));
  EXPECT_EQ(3, path.getVertices().count());
  EXPECT_EQ(p1, path.getVertices().value(0).getPos());
  EXPECT_EQ(-Angle::deg180(), path.getVertices().value(0).getAngle());
  EXPECT_EQ(p2, path.getVertices().value(1).getPos());
  EXPECT_EQ(-Angle::deg180(), path.getVertices().value(1).getAngle());
  EXPECT_EQ(p1, path.getVertices().value(2).getPos());
  EXPECT_EQ(Angle(0), path.getVertices().value(2).getAngle());
  EXPECT_TRUE(path.isClosed());
}

TEST_F(PathTest, testDonut) {
  const PositiveLength outerDiameter(1000);
  const PositiveLength innerDiameter(500);

  const Point ot(Length(0), Length(500));
  const Point ob(Length(0), Length(-500));
  const Point it(Length(0), Length(250));
  const Point ib(Length(0), Length(-250));
  Path expected;
  expected.addVertex(ot, -Angle::deg180());
  expected.addVertex(ob, Angle::deg0());
  expected.addVertex(ib, Angle::deg180());
  expected.addVertex(it, Angle::deg180());
  expected.addVertex(ib, Angle::deg0());
  expected.addVertex(ob, -Angle::deg180());
  expected.addVertex(ot, Angle::deg0());

  const Path actual = Path::donut(outerDiameter, innerDiameter);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testDonutInvalid) {
  const PositiveLength outerDiameter(1000);
  const PositiveLength innerDiameter = outerDiameter;

  const Path expected;  // Empty.
  const Path actual = Path::donut(outerDiameter, innerDiameter);

  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(PathTest, testCenteredRectRoundedCorners) {
  Path expected = Path({
      Vertex(Point(-30000, 75000), Angle::deg0()),
      Vertex(Point(30000, 75000), -Angle::deg90()),
      Vertex(Point(50000, 55000), Angle::deg0()),
      Vertex(Point(50000, -55000), -Angle::deg90()),
      Vertex(Point(30000, -75000), Angle::deg0()),
      Vertex(Point(-30000, -75000), -Angle::deg90()),
      Vertex(Point(-50000, -55000), Angle::deg0()),
      Vertex(Point(-50000, 55000), -Angle::deg90()),
      Vertex(Point(-30000, 75000), Angle::deg0()),
  });
  Path actual = Path::centeredRect(
      PositiveLength(100000), PositiveLength(150000), UnsignedLength(20000));
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(PathTest, testCenteredRectRoundedCornersSaturation) {
  Path expected = Path::obround(PositiveLength(100000), PositiveLength(150000));
  Path actual = Path::centeredRect(
      PositiveLength(100000), PositiveLength(150000), UnsignedLength(60000));
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(PathTest, testChamferedRect) {
  const PositiveLength w(1000);
  const PositiveLength h(500);
  const UnsignedLength c(100);

  Path expected;
  expected.addVertex(Point(-500, 150), Angle::deg0());
  expected.addVertex(Point(-400, 250), Angle::deg0());
  expected.addVertex(Point(400, 250), Angle::deg0());
  expected.addVertex(Point(500, 150), Angle::deg0());
  expected.addVertex(Point(500, -150), Angle::deg0());
  expected.addVertex(Point(400, -250), Angle::deg0());
  expected.addVertex(Point(-400, -250), Angle::deg0());
  expected.addVertex(Point(-500, -150), Angle::deg0());
  expected.addVertex(Point(-500, 150), Angle::deg0());

  const Path actual = Path::chamferedRect(w, h, c, true, true, true, true);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testChamferedRectTopRight) {
  const PositiveLength w(1000);
  const PositiveLength h(500);
  const UnsignedLength c(100);

  Path expected;
  expected.addVertex(Point(-500, 250), Angle::deg0());
  expected.addVertex(Point(400, 250), Angle::deg0());
  expected.addVertex(Point(500, 150), Angle::deg0());
  expected.addVertex(Point(500, -250), Angle::deg0());
  expected.addVertex(Point(-500, -250), Angle::deg0());
  expected.addVertex(Point(-500, 250), Angle::deg0());

  const Path actual = Path::chamferedRect(w, h, c, false, true, false, false);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testChamferedRectSaturated) {
  const PositiveLength w(1000);
  const PositiveLength h(500);
  const UnsignedLength c(300);  // Must be clipped to 250.

  Path expected;
  expected.addVertex(Point(-500, 0), Angle::deg0());
  expected.addVertex(Point(-250, 250), Angle::deg0());
  expected.addVertex(Point(250, 250), Angle::deg0());
  expected.addVertex(Point(500, 0), Angle::deg0());
  expected.addVertex(Point(250, -250), Angle::deg0());
  expected.addVertex(Point(-250, -250), Angle::deg0());
  expected.addVertex(Point(-500, 0), Angle::deg0());

  const Path actual = Path::chamferedRect(w, h, c, true, true, true, true);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testTrapezoid) {
  const PositiveLength w(1000);
  const PositiveLength h(800);
  const Length dw(200);
  const Length dh(100);

  Path expected;
  expected.addVertex(Point(-600, 350), Angle::deg0());
  expected.addVertex(Point(600, 450), Angle::deg0());
  expected.addVertex(Point(400, -450), Angle::deg0());
  expected.addVertex(Point(-400, -350), Angle::deg0());
  expected.addVertex(Point(-600, 350), Angle::deg0());

  const Path actual = Path::trapezoid(w, h, dw, dh);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

// Note: This is actually a very strange case, in real world we'll probably
// never create such trapezoids which do not look like a trapezoid anymore.
TEST_F(PathTest, testTrapezoidSaturated) {
  const PositiveLength w(1000);
  const PositiveLength h(800);
  const Length dw(1200);  // Must be clipped to 1000.
  const Length dh(-1200);  // Must be clipped to -800.

  Path expected;
  expected.addVertex(Point(-1000, 800), Angle::deg0());
  expected.addVertex(Point(1000, 0), Angle::deg0());
  expected.addVertex(Point(0, 0), Angle::deg0());
  expected.addVertex(Point(0, -800), Angle::deg0());
  expected.addVertex(Point(-1000, 800), Angle::deg0());

  const Path actual = Path::trapezoid(w, h, dw, dh);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testTrapezoidDw) {
  const PositiveLength w(1000);
  const PositiveLength h(800);
  const Length dw(200);
  const Length dh(0);

  Path expected;
  expected.addVertex(Point(-600, 400), Angle::deg0());
  expected.addVertex(Point(600, 400), Angle::deg0());
  expected.addVertex(Point(400, -400), Angle::deg0());
  expected.addVertex(Point(-400, -400), Angle::deg0());
  expected.addVertex(Point(-600, 400), Angle::deg0());

  const Path actual = Path::trapezoid(w, h, dw, dh);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testTrapezoidDh) {
  const PositiveLength w(1000);
  const PositiveLength h(800);
  const Length dw(0);
  const Length dh(-200);

  Path expected;
  expected.addVertex(Point(-500, 500), Angle::deg0());
  expected.addVertex(Point(500, 300), Angle::deg0());
  expected.addVertex(Point(500, -300), Angle::deg0());
  expected.addVertex(Point(-500, -500), Angle::deg0());
  expected.addVertex(Point(-500, 500), Angle::deg0());

  const Path actual = Path::trapezoid(w, h, dw, dh);

  EXPECT_EQ(str(expected), str(actual));
  EXPECT_TRUE(actual.isClosed());
}

TEST_F(PathTest, testOctagonRoundedCornersSaturation) {
  Path expected = Path::obround(PositiveLength(100000), PositiveLength(150000));
  Path actual = Path::octagon(PositiveLength(100000), PositiveLength(150000),
                              UnsignedLength(60000));
  EXPECT_EQ(str(expected), str(actual));
}

// Test to reproduce https://github.com/LibrePCB/LibrePCB/issues/974
TEST_F(PathTest, testFlatArc) {
  Path expected = Path({
      Vertex(Point(30875000, 32385000)),
      Vertex(Point(29725000, 30393142)),
      Vertex(Point(27425000, 30393142)),
      Vertex(Point(26275000, 32385000)),
  });
  Path actual =
      Path::flatArc(Point(30875000, 32385000), Point(26275000, 32385000),
                    -Angle::deg180(), PositiveLength(1000000));
  EXPECT_EQ(str(expected), str(actual));
}

// Test to reproduce another case where small deviations were observed
TEST_F(PathTest, testFlatArc2) {
  Path expected = Path({
      Vertex(Point(-21401446, 16018901)),
      Vertex(Point(-22394290, 15545339)),
      Vertex(Point(-23300829, 16168386)),
      Vertex(Point(-23214523, 17264994)),
  });
  Path actual =
      Path::flatArc(Point(-21401446, 16018901), Point(-23214523, 17264994),
                    -Angle::deg180(), PositiveLength(2000000));
  EXPECT_EQ(str(expected), str(actual));
}

/*******************************************************************************
 *  Parametrized obround(width, height) Tests
 ******************************************************************************/

struct PathObroundWidthHeightTestData {
  PositiveLength width;
  PositiveLength height;
  QList<QPair<Point, Angle>> vertices;
};

class PathObroundWidthHeightTest
  : public PathTest,
    public ::testing::WithParamInterface<PathObroundWidthHeightTestData> {};

TEST_P(PathObroundWidthHeightTest, test) {
  const PathObroundWidthHeightTestData& data = GetParam();

  Path path = Path::obround(data.width, data.height);
  EXPECT_EQ(data.vertices.count(), path.getVertices().count());
  for (int i = 0; i < data.vertices.count(); ++i) {
    EXPECT_EQ(data.vertices.value(i).first,
              path.getVertices().value(i).getPos());
    EXPECT_EQ(data.vertices.value(i).second,
              path.getVertices().value(i).getAngle());
  }
  EXPECT_TRUE(path.isClosed());
}

// clang-format off
static PathObroundWidthHeightTestData sObroundWidthHeightTestData[] = {
  { // width == height
    PositiveLength(10), PositiveLength(10),
    {
      {Point(Length(5), Length(0)), -Angle::deg180()},
      {Point(Length(-5), Length(0)), -Angle::deg180()},
      {Point(Length(5), Length(0)), Angle::deg0()},
    },
  },
  { // width > height
    PositiveLength(30), PositiveLength(10),
    {
      {Point(Length(-10), Length(5)), Angle::deg0()},
      {Point(Length(10), Length(5)), -Angle::deg180()},
      {Point(Length(10), Length(-5)), Angle::deg0()},
      {Point(Length(-10), Length(-5)), -Angle::deg180()},
      {Point(Length(-10), Length(5)), Angle::deg0()},
    },
  },
  { // width < height
    PositiveLength(10), PositiveLength(30),
    {
      {Point(Length(5), Length(10)), Angle::deg0()},
      {Point(Length(5), Length(-10)), -Angle::deg180()},
      {Point(Length(-5), Length(-10)), Angle::deg0()},
      {Point(Length(-5), Length(10)), -Angle::deg180()},
      {Point(Length(5), Length(10)), Angle::deg0()},
    },
  },
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(PathObroundWidthHeightTest, PathObroundWidthHeightTest,
                         ::testing::ValuesIn(sObroundWidthHeightTestData));

/*******************************************************************************
 *  Parametrized obround(p1, p2, width) Tests
 ******************************************************************************/

struct PathObroundP1P2WidthTestData {
  Point p1;
  Point p2;
  PositiveLength width;
  QList<QPair<Point, Angle>> vertices;
};

class PathObroundP1P2WidthTest
  : public PathTest,
    public ::testing::WithParamInterface<PathObroundP1P2WidthTestData> {};

TEST_P(PathObroundP1P2WidthTest, test) {
  const PathObroundP1P2WidthTestData& data = GetParam();

  Path path = Path::obround(data.p1, data.p2, data.width);
  EXPECT_EQ(data.vertices.count(), path.getVertices().count());
  for (int i = 0; i < data.vertices.count(); ++i) {
    EXPECT_EQ(data.vertices.value(i).first,
              path.getVertices().value(i).getPos());
    EXPECT_EQ(data.vertices.value(i).second,
              path.getVertices().value(i).getAngle());
  }
  EXPECT_TRUE(path.isClosed());
}

// clang-format off
static PathObroundP1P2WidthTestData sObroundP1P2WidthTestData[] = {
  { // on x-axis from negative to positive
    Point(Length(-10), Length(0)),
    Point(Length(10), Length(0)),
    PositiveLength(20),
    {
      {Point(Length(-10), Length(10)), Angle::deg0()},
      {Point(Length(10), Length(10)), -Angle::deg180()},
      {Point(Length(10), Length(-10)), Angle::deg0()},
      {Point(Length(-10), Length(-10)), -Angle::deg180()},
      {Point(Length(-10), Length(10)), Angle::deg0()},
    },
  },
  { // horizontal from positive to negative
    Point(Length(10), Length(55)),
    Point(Length(-10), Length(55)),
    PositiveLength(2),
    {
      {Point(Length(10), Length(54)), Angle::deg0()},
      {Point(Length(-10), Length(54)), -Angle::deg180()},
      {Point(Length(-10), Length(56)), Angle::deg0()},
      {Point(Length(10), Length(56)), -Angle::deg180()},
      {Point(Length(10), Length(54)), Angle::deg0()},
    },
  },
  { // on y-axis from negative to positive
    Point(Length(0), Length(-20)),
    Point(Length(0), Length(-10)),
    PositiveLength(2),
    {
      {Point(Length(-1), Length(-20)), Angle::deg0()},
      {Point(Length(-1), Length(-10)), -Angle::deg180()},
      {Point(Length(1), Length(-10)), Angle::deg0()},
      {Point(Length(1), Length(-20)), -Angle::deg180()},
      {Point(Length(-1), Length(-20)), Angle::deg0()},
    },
  },
  { // vertical from positive to negative
    Point(Length(-5), Length(-10)),
    Point(Length(-5), Length(-20)),
    PositiveLength(2),
    {
      {Point(Length(-4), Length(-10)), Angle::deg0()},
      {Point(Length(-4), Length(-20)), -Angle::deg180()},
      {Point(Length(-6), Length(-20)), Angle::deg0()},
      {Point(Length(-6), Length(-10)), -Angle::deg180()},
      {Point(Length(-4), Length(-10)), Angle::deg0()},
    },
  },
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(PathObroundP1P2WidthTest, PathObroundP1P2WidthTest,
                         ::testing::ValuesIn(sObroundP1P2WidthTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
