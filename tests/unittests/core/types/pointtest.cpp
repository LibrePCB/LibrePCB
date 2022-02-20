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
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class PointTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(PointTest, testDefaultConstructor) {
  Point p;
  EXPECT_EQ(0, p.getX().toNm());
  EXPECT_EQ(0, p.getY().toNm());
  EXPECT_TRUE(p.isOrigin());
}

TEST(PointTest, testOperatorLessThan) {
  EXPECT_FALSE(Point(Length(0), Length(0)) < Point(Length(0), Length(0)));
  EXPECT_FALSE(Point(Length(10), Length(20)) < Point(Length(9), Length(19)));
  EXPECT_FALSE(Point(Length(10), Length(20)) < Point(Length(9), Length(21)));
  EXPECT_FALSE(Point(Length(10), Length(20)) < Point(Length(10), Length(19)));
  EXPECT_FALSE(Point(Length(-10), Length(20)) < Point(Length(-11), Length(0)));
  EXPECT_TRUE(Point(Length(0), Length(0)) < Point(Length(0), Length(1)));
  EXPECT_TRUE(Point(Length(0), Length(0)) < Point(Length(1), Length(0)));
  EXPECT_TRUE(Point(Length(10), Length(20)) < Point(Length(11), Length(19)));
  EXPECT_TRUE(Point(Length(10), Length(20)) < Point(Length(11), Length(21)));
  EXPECT_TRUE(Point(Length(10), Length(20)) < Point(Length(10), Length(21)));
  EXPECT_TRUE(Point(Length(-1), Length(-2)) < Point(Length(-1), Length(-1)));
}

TEST(PointTest, testOperatorLessEqual) {
  EXPECT_FALSE(Point(Length(10), Length(20)) <= Point(Length(9), Length(19)));
  EXPECT_FALSE(Point(Length(10), Length(20)) <= Point(Length(9), Length(21)));
  EXPECT_FALSE(Point(Length(10), Length(20)) <= Point(Length(10), Length(19)));
  EXPECT_FALSE(Point(Length(-10), Length(20)) <= Point(Length(-11), Length(0)));
  EXPECT_TRUE(Point(Length(0), Length(0)) <= Point(Length(0), Length(0)));
  EXPECT_TRUE(Point(Length(0), Length(0)) <= Point(Length(0), Length(1)));
  EXPECT_TRUE(Point(Length(0), Length(0)) <= Point(Length(1), Length(0)));
  EXPECT_TRUE(Point(Length(10), Length(20)) <= Point(Length(11), Length(19)));
  EXPECT_TRUE(Point(Length(10), Length(20)) <= Point(Length(11), Length(21)));
  EXPECT_TRUE(Point(Length(10), Length(20)) <= Point(Length(10), Length(21)));
  EXPECT_TRUE(Point(Length(-1), Length(-2)) <= Point(Length(-1), Length(-1)));
}

TEST(PointTest, testOperatorGreaterThan) {
  EXPECT_FALSE(Point(Length(0), Length(0)) > Point(Length(0), Length(0)));
  EXPECT_FALSE(Point(Length(0), Length(0)) > Point(Length(0), Length(1)));
  EXPECT_FALSE(Point(Length(0), Length(0)) > Point(Length(1), Length(0)));
  EXPECT_FALSE(Point(Length(10), Length(20)) > Point(Length(11), Length(19)));
  EXPECT_FALSE(Point(Length(10), Length(20)) > Point(Length(11), Length(21)));
  EXPECT_FALSE(Point(Length(10), Length(20)) > Point(Length(10), Length(21)));
  EXPECT_FALSE(Point(Length(-1), Length(-2)) > Point(Length(-1), Length(-1)));
  EXPECT_TRUE(Point(Length(10), Length(20)) > Point(Length(9), Length(19)));
  EXPECT_TRUE(Point(Length(10), Length(20)) > Point(Length(9), Length(21)));
  EXPECT_TRUE(Point(Length(10), Length(20)) > Point(Length(10), Length(19)));
  EXPECT_TRUE(Point(Length(-10), Length(20)) > Point(Length(-11), Length(0)));
}

TEST(PointTest, testOperatorGreaterEqual) {
  EXPECT_FALSE(Point(Length(0), Length(0)) >= Point(Length(0), Length(1)));
  EXPECT_FALSE(Point(Length(0), Length(0)) >= Point(Length(1), Length(0)));
  EXPECT_FALSE(Point(Length(10), Length(20)) >= Point(Length(11), Length(19)));
  EXPECT_FALSE(Point(Length(10), Length(20)) >= Point(Length(11), Length(21)));
  EXPECT_FALSE(Point(Length(10), Length(20)) >= Point(Length(10), Length(21)));
  EXPECT_FALSE(Point(Length(-1), Length(-2)) >= Point(Length(-1), Length(-1)));
  EXPECT_TRUE(Point(Length(0), Length(0)) >= Point(Length(0), Length(0)));
  EXPECT_TRUE(Point(Length(10), Length(20)) >= Point(Length(9), Length(19)));
  EXPECT_TRUE(Point(Length(10), Length(20)) >= Point(Length(9), Length(21)));
  EXPECT_TRUE(Point(Length(10), Length(20)) >= Point(Length(10), Length(19)));
  EXPECT_TRUE(Point(Length(-10), Length(20)) >= Point(Length(-11), Length(0)));
}

/*******************************************************************************
 *  Tests for rotate()
 ******************************************************************************/

struct PointRotateTestData {
  Point input;
  Angle angle;
  Point center;
  Point output;
};

class PointRotateTest : public ::testing::TestWithParam<PointRotateTestData> {};

TEST_P(PointRotateTest, test) {
  const PointRotateTestData& data = GetParam();

  Point p(data.input);
  p.rotate(data.angle, data.center);
  EXPECT_EQ(data.output, p);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(PointRotateTest, PointRotateTest, ::testing::Values(
    //                  {input,                  angle,               center,                 output}
    PointRotateTestData({Point::fromMm(10, 0),   Angle::fromDeg(0),   Point::fromMm(0, 0),    Point::fromMm(10, 0)  }),
    PointRotateTestData({Point::fromMm(10, 0),   Angle::fromDeg(180), Point::fromMm(0, 0),    Point::fromMm(-10, 0) }),
    PointRotateTestData({Point::fromMm(10, 0),   Angle::fromDeg(270), Point::fromMm(0, 0),    Point::fromMm(0, -10) }),
    PointRotateTestData({Point::fromMm(10, 0),   Angle::fromDeg(0),   Point::fromMm(0, 0),    Point::fromMm(10, 0)  }),

    PointRotateTestData({Point::fromMm(0, 0),    Angle::fromDeg(90),  Point::fromMm(0, 0),    Point::fromMm(0, 0)   }),
    PointRotateTestData({Point::fromMm(10, 0),   Angle::fromDeg(90),  Point::fromMm(0, 0),    Point::fromMm(0, 10)  }),
    PointRotateTestData({Point::fromMm(0, 10),   Angle::fromDeg(90),  Point::fromMm(0, 0),    Point::fromMm(-10, 0) }),
    PointRotateTestData({Point::fromMm(-10, 0),  Angle::fromDeg(90),  Point::fromMm(0, 0),    Point::fromMm(0, -10) }),
    PointRotateTestData({Point::fromMm(0, -10),  Angle::fromDeg(90),  Point::fromMm(0, 0),    Point::fromMm(10, 0)  }),

    PointRotateTestData({Point::fromMm(100, 50), Angle::fromDeg(90),  Point::fromMm(100, 50), Point::fromMm(100, 50)}),
    PointRotateTestData({Point::fromMm(110, 50), Angle::fromDeg(90),  Point::fromMm(100, 50), Point::fromMm(100, 60)}),
    PointRotateTestData({Point::fromMm(100, 60), Angle::fromDeg(90),  Point::fromMm(100, 50), Point::fromMm(90, 50) }),
    PointRotateTestData({Point::fromMm(90, 50),  Angle::fromDeg(90),  Point::fromMm(100, 50), Point::fromMm(100, 40)}),
    PointRotateTestData({Point::fromMm(100, 40), Angle::fromDeg(90),  Point::fromMm(100, 50), Point::fromMm(110, 50)}),

    PointRotateTestData({Point(10, 0),           Angle::fromDeg(1),   Point(0, 0),            Point(10, 0)  }),
    PointRotateTestData({Point(10, 0),           Angle::fromDeg(2),   Point(0, 0),            Point(10, 0)  }),
    PointRotateTestData({Point(10, 0),           Angle::fromDeg(3),   Point(0, 0),            Point(10, 1)  }),
    PointRotateTestData({Point(10, 0),           Angle::fromDeg(4),   Point(0, 0),            Point(10, 1)  }),
    PointRotateTestData({Point(10, 0),           Angle::fromDeg(18),  Point(0, 0),            Point(10, 3)  }),
    PointRotateTestData({Point(10, 0),           Angle::fromDeg(19),  Point(0, 0),            Point(9, 3)   })
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
