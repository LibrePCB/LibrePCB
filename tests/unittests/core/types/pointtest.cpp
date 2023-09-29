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
 *  Tests for precision of getLength()
 ******************************************************************************/

struct PointLengthPrecisionTestData {
  Length range;
  UnsignedLength axisLength;
  UnsignedLength squaredLength;
};

class PointLengthPrecisionTest
  : public ::testing::TestWithParam<PointLengthPrecisionTestData> {};

TEST_P(PointLengthPrecisionTest, testPrecision) {
  const PointLengthPrecisionTestData& data = GetParam();

  Point px(data.range, 0);
  Point py(0, data.range);
  Point pxy(data.range, data.range);

  ASSERT_NO_THROW(px.getLength());
  ASSERT_NO_THROW(py.getLength());
  ASSERT_NO_THROW(pxy.getLength());

  EXPECT_EQ(data.axisLength, px.getLength())
      << "Computed: " << px.getLength()->toNm()
      << " Expected: " << data.axisLength->toNm();
  EXPECT_EQ(data.axisLength, py.getLength())
      << "Computed: " << px.getLength()->toNm()
      << " Expected: " << data.axisLength->toNm();
  EXPECT_EQ(data.squaredLength, pxy.getLength())
      << "Computed: " << pxy.getLength()->toNm()
      << " Expected: " << data.squaredLength->toNm();
}

// sqrt2 according wikipedia
// 1.41421356237309504880168872420969807856967187537694807317667973799

static const LengthBase_t MM = 1000000LL;
static const LengthBase_t M = 1000000000LL;
static const LengthBase_t KM = 1000000000000LL;

// clang-format off
INSTANTIATE_TEST_SUITE_P(PointLengthPrecisionTest, PointLengthPrecisionTest, ::testing::Values(
    //                           {range,            axisLength,               squaredLength}
    PointLengthPrecisionTestData({Length(0),        UnsignedLength(0),        UnsignedLength(0)}),

    // keep precision on nanometer scale
    PointLengthPrecisionTestData({Length(10),       UnsignedLength(10),       UnsignedLength(14)}),

    // keep precision on millimeter scale
    PointLengthPrecisionTestData({Length(MM),       UnsignedLength(MM),       UnsignedLength(1414213)}),

    // keep precision on meter scale
    PointLengthPrecisionTestData({Length(M),        UnsignedLength(M),        UnsignedLength(1414213562)}),
    PointLengthPrecisionTestData({Length(2*M),      UnsignedLength(2*M),      UnsignedLength(2828427124)}),
    PointLengthPrecisionTestData({Length(3*M),      UnsignedLength(3*M),      UnsignedLength(4242640687)}),

    // keep precision on kilometer scale
    PointLengthPrecisionTestData({Length(KM),       UnsignedLength(KM),       UnsignedLength(1414213562373)}),
    PointLengthPrecisionTestData({Length(10*KM),    UnsignedLength(10*KM),    UnsignedLength(14142135623730)}),
    PointLengthPrecisionTestData({Length(100*KM),   UnsignedLength(100*KM),   UnsignedLength(141421356237309)}),

    // keep precision on small planet's scale
    PointLengthPrecisionTestData({Length(1000*KM),  UnsignedLength(1000*KM),  UnsignedLength(1414213562373095)})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
