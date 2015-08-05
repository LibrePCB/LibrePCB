/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <gtest/gtest.h>
#include <librepcbcommon/units/point.h>
#include <librepcbcommon/units/angle.h>

namespace tests {

/*****************************************************************************************
 *  Test Data Type
 ****************************************************************************************/

typedef struct {
    Point pA;
    Point pB;
    Point pCenter;
    Angle aRot;
} PointTestData_t;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class PointTest : public ::testing::TestWithParam<PointTestData_t>
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_P(PointTest, testDefaultConstructor)
{
    Point p;
    EXPECT_EQ(0, p.getX().toNm());
    EXPECT_EQ(0, p.getY().toNm());
    EXPECT_TRUE(p.isOrigin());
}

TEST_P(PointTest, testRotate)
{
    const PointTestData_t& data = GetParam();

    Point p(data.pA);
    p.rotate(data.aRot, data.pCenter);
    EXPECT_EQ(data.pB, p);
}

/*****************************************************************************************
 *  Test Data
 ****************************************************************************************/

INSTANTIATE_TEST_CASE_P(PointTest, PointTest, ::testing::Values(
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
