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
#include <librepcb/common/geometry/path.h>

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
  PathTest() {}
  virtual ~PathTest() {}
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(PathTest, testDefaultConstructorCreatesEmptyPath) {
  Path path;
  EXPECT_EQ(0, path.getVertices().count());
}

TEST_F(PathTest, testLine) {
  Point p1(Length(12), Length(34));
  Point p2(Length(56), Length(78));
  Angle angle(1234);
  Path  path = Path::line(p1, p2, angle);
  EXPECT_EQ(2, path.getVertices().count());
  EXPECT_EQ(p1, path.getVertices().value(0).getPos());
  EXPECT_EQ(angle, path.getVertices().value(0).getAngle());
  EXPECT_EQ(p2, path.getVertices().value(1).getPos());
  EXPECT_EQ(Angle(0), path.getVertices().value(1).getAngle());
  EXPECT_FALSE(path.isClosed());
}

TEST_F(PathTest, testCircle) {
  PositiveLength diameter(1000);
  Path           path = Path::circle(diameter);
  Point          p1(Length(500), Length(0));
  Point          p2(Length(-500), Length(0));
  EXPECT_EQ(3, path.getVertices().count());
  EXPECT_EQ(p1, path.getVertices().value(0).getPos());
  EXPECT_EQ(-Angle::deg180(), path.getVertices().value(0).getAngle());
  EXPECT_EQ(p2, path.getVertices().value(1).getPos());
  EXPECT_EQ(-Angle::deg180(), path.getVertices().value(1).getAngle());
  EXPECT_EQ(p1, path.getVertices().value(2).getPos());
  EXPECT_EQ(Angle(0), path.getVertices().value(2).getAngle());
  EXPECT_TRUE(path.isClosed());
}

/*******************************************************************************
 *  Parametrized obround(width, height) Tests
 ******************************************************************************/

struct PathObroundWidthHeightTestData {
  PositiveLength             width;
  PositiveLength             height;
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

INSTANTIATE_TEST_CASE_P(PathObroundWidthHeightTest, PathObroundWidthHeightTest,
                        ::testing::ValuesIn(sObroundWidthHeightTestData));

/*******************************************************************************
 *  Parametrized obround(p1, p2, width) Tests
 ******************************************************************************/

struct PathObroundP1P2WidthTestData {
  Point                      p1;
  Point                      p2;
  PositiveLength             width;
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

INSTANTIATE_TEST_CASE_P(PathObroundP1P2WidthTest, PathObroundP1P2WidthTest,
                        ::testing::ValuesIn(sObroundP1P2WidthTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
