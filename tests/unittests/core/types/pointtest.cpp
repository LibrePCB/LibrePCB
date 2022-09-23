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
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  Point pA;
  Point pB;
  Point pCenter;
  Angle aRot;
} PointTestData_t;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class PointTest : public ::testing::TestWithParam<PointTestData_t> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(PointTest, testDefaultConstructor) {
  Point p;
  EXPECT_EQ(0, p.getX().toNm());
  EXPECT_EQ(0, p.getY().toNm());
  EXPECT_TRUE(p.isOrigin());
}

TEST_P(PointTest, testRotate) {
  const PointTestData_t& data = GetParam();

  Point p(data.pA);
  p.rotate(data.aRot, data.pCenter);
  EXPECT_EQ(data.pB, p);
}

TEST_P(PointTest, testOperatorLessThan) {
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

TEST_P(PointTest, testOperatorLessEqual) {
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

TEST_P(PointTest, testOperatorGreaterThan) {
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

TEST_P(PointTest, testOperatorGreaterEqual) {
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
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(PointTest, PointTest, ::testing::Values(
    //              {pA,                     pB,                     pCenter,                aRot}
    PointTestData_t({Point::fromMm(0, 0),    Point::fromMm(0, 0),    Point::fromMm(0, 0),    Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(10, 0),   Point::fromMm(0, 10),   Point::fromMm(0, 0),    Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(0, 10),   Point::fromMm(-10, 0),  Point::fromMm(0, 0),    Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(-10, 0),  Point::fromMm(0, -10),  Point::fromMm(0, 0),    Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(0, -10),  Point::fromMm(10, 0),   Point::fromMm(0, 0),    Angle::fromDeg(90)}),

    PointTestData_t({Point::fromMm(100, 50), Point::fromMm(100, 50), Point::fromMm(100, 50), Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(110, 50), Point::fromMm(100, 60), Point::fromMm(100, 50), Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(100, 60), Point::fromMm(90, 50),  Point::fromMm(100, 50), Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(90, 50),  Point::fromMm(100, 40), Point::fromMm(100, 50), Angle::fromDeg(90)}),
    PointTestData_t({Point::fromMm(100, 40), Point::fromMm(110, 50), Point::fromMm(100, 50), Angle::fromDeg(90)})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
