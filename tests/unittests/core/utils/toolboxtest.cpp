/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2017 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ToolboxTest : public ::testing::Test {};

/*******************************************************************************
 *  isTextUpsideDown() Tests
 ******************************************************************************/

TEST_F(ToolboxTest, testIsTextUpsideDown) {
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(-360000000)));  // 0°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(-315000000)));  // 45°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(-270000000)));  // 90°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(-225000000)));  // 135°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(-180000000)));  // 180°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(-135000000)));  // 225°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(-90000000)));  // 270°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(-45000000)));  // 315°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(0)));  // 0°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(45000000)));  // 45°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(90000000)));  // 90°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(135000000)));  // 135°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(180000000)));  // 180°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(225000000)));  // 225°
  EXPECT_TRUE(Toolbox::isTextUpsideDown(Angle(270000000)));  // 270°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(315000000)));  // 315°
  EXPECT_FALSE(Toolbox::isTextUpsideDown(Angle(360000000)));  // 0°
}

/*******************************************************************************
 *  shapeFromPath() Tests
 ******************************************************************************/

TEST_F(ToolboxTest, testNoPenReturnsUnmodifiedPath) {
  QPainterPath path;
  path.addRect(10, 20, 30, 40);
  QPen pen(QBrush(Qt::SolidPattern), 1, Qt::NoPen);
  QBrush brush(Qt::SolidPattern);
  EXPECT_EQ(path, Toolbox::shapeFromPath(path, pen, brush));
}

TEST_F(ToolboxTest, testNoPenBrushReturnsUnmodifiedPath) {
  QPainterPath path;
  path.addRect(10, 20, 30, 40);
  QPen pen(QBrush(Qt::NoBrush), 1, Qt::SolidLine);
  QBrush brush(Qt::SolidPattern);
  EXPECT_EQ(path, Toolbox::shapeFromPath(path, pen, brush));
}

/*******************************************************************************
 *  Parametrized arcCenter() Tests
 ******************************************************************************/

struct ToolboxArcCenterTestData {
  Point p1;
  Point p2;
  Angle angle;
  Point center;
};

class ToolboxArcCenterTest
  : public ToolboxTest,
    public ::testing::WithParamInterface<ToolboxArcCenterTestData> {};

TEST_P(ToolboxArcCenterTest, test) {
  const ToolboxArcCenterTestData& data = GetParam();

  // On Windows, abort here and skip this test because on AppVeyor the result
  // is slightly different. See discussion here:
  // https://github.com/LibrePCB/LibrePCB/pull/511#issuecomment-529089212
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
  GTEST_SKIP();
#endif

  EXPECT_EQ(data.center, Toolbox::arcCenter(data.p1, data.p2, data.angle));
}

// clang-format off
static ToolboxArcCenterTestData sToolboxArcCenterTestData[] = {
// p1,                         p2,                         angle,              center
  {Point(47744137, 37820591),  Point(55364137, 24622364),  -Angle::deg90(),    Point(44955023, 27411478)},
  // Test to reproduce https://github.com/LibrePCB/LibrePCB/issues/974
  {Point(30875000, 32385000),  Point(26275000, 32385000),  -Angle::deg180(),   Point(28575000, 32385000)}
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(ToolboxArcCenterTest, ToolboxArcCenterTest,
                         ::testing::ValuesIn(sToolboxArcCenterTestData));

/*******************************************************************************
 *  Parametrized arcAngle() Tests
 ******************************************************************************/

struct ToolboxArcAngleTestData {
  Point p1;
  Point p2;
  Point center;
  Angle angle;
};

class ToolboxArcAngleTest
  : public ToolboxTest,
    public ::testing::WithParamInterface<ToolboxArcAngleTestData> {};

TEST_P(ToolboxArcAngleTest, test) {
  const ToolboxArcAngleTestData& data = GetParam();

  EXPECT_EQ(data.angle.toDegString().toStdString(),
            Toolbox::arcAngle(data.p1, data.p2, data.center)
                .toDegString()
                .toStdString());
}

// clang-format off
static ToolboxArcAngleTestData sToolboxArcAngleTestData[] = {
// p1,                        p2,                       center,                   angle
  {Point(0, 0),               Point(0, 0),              Point(0, 0),              Angle::deg0()  },
  {Point(2000000, 0),         Point(1000000, 0),        Point(0, 0),              Angle::deg0()  },
  {Point(2000000, 0),         Point(-1000000, 0),       Point(0, 0),              Angle::deg180()},
  {Point(2000000, 3000000),   Point(-1000000, 2000000), Point(1000000, 1000000),  Angle::deg90() },
  {Point(-1000000, 2000000),  Point(2000000, 3000000),  Point(1000000, 1000000),  Angle::deg270()},
  {Point(2000000, 3000000),   Point(3000000, 0),        Point(1000000, 1000000),  Angle::deg270()},
  {Point(3000000, 0),         Point(2000000, 3000000),  Point(1000000, 1000000),  Angle::deg90() }
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(ToolboxArcAngleTest, ToolboxArcAngleTest,
                         ::testing::ValuesIn(sToolboxArcAngleTestData));

/*******************************************************************************
 *  Parametrized incrementNumberInString() Tests
 ******************************************************************************/

struct ToolboxIncrementNumberInStringTestData {
  QString input;
  QString output;
};

class ToolboxIncrementNumberInStringTest
  : public ToolboxTest,
    public ::testing::WithParamInterface<
        ToolboxIncrementNumberInStringTestData> {};

TEST_P(ToolboxIncrementNumberInStringTest, test) {
  const ToolboxIncrementNumberInStringTestData& data = GetParam();

  EXPECT_EQ(data.output, Toolbox::incrementNumberInString(data.input));
}

// clang-format off
static ToolboxIncrementNumberInStringTestData
    sToolboxIncrementNumberInStringTestData[] = {
// input,                     output
  {"",                        "1"},
  {"  ",                      "  1"},
  {"0",                       "1"},
  {"1",                       "2"},
  {" 123 ",                   " 124 "},
  {"X",                       "X1"},
  {"X-1",                     "X-2"},
  {"GND 41",                  "GND 42"},
  {"FOO1.2",                  "FOO1.3"},
  {"12 foo 34",               "12 foo 35"},
  {"12 foo 34 bar 56 ",       "12 foo 34 bar 57 "},
  {"99A",                     "100A"},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(
    ToolboxIncrementNumberInStringTest, ToolboxIncrementNumberInStringTest,
    ::testing::ValuesIn(sToolboxIncrementNumberInStringTestData));

/*******************************************************************************
 *  Parametrized expandRangesInString() Tests
 ******************************************************************************/

struct ToolboxExpandRangesInStringTestData {
  QString input;
  QStringList output;
};

class ToolboxExpandRangesInStringTest
  : public ToolboxTest,
    public ::testing::WithParamInterface<ToolboxExpandRangesInStringTestData> {
};

TEST_P(ToolboxExpandRangesInStringTest, test) {
  const ToolboxExpandRangesInStringTestData& data = GetParam();

  QStringList actual = Toolbox::expandRangesInString(data.input);

  QString msg;
  QDebug dbg(&msg);
  dbg << data.input << "->" << actual << "!=" << data.output;
  EXPECT_EQ(data.output, actual) << msg.toStdString();
}

// clang-format off
static ToolboxExpandRangesInStringTestData
    sToolboxExpandRangesInStringTestData[] = {
// input,             output
  {"",                {""}},
  {"  ",              {"  "}},
  {"..",              {".."}},
  {"1",               {"1"}},
  {"A..A",            {"A"}},
  {"1..5",            {"1", "2", "3", "4", "5"}},
  {"X-2..2",          {"X-2"}},
  {"X-5..11",         {"X-5", "X-6", "X-7", "X-8", "X-9", "X-10", "X-11"}},
  {"X3..-1Y",         {"X3..-1Y"}},
  {"0..1_X..Y",       {"0_X", "0_Y", "1_X", "1_Y"}},
  {"-1..3_z..y",      {"-1_z", "-1_y", "-2_z", "-2_y", "-3_z", "-3_y"}},
  {"2..3B..A0..1",    {"2B0", "2B1", "2A0", "2A1", "3B0", "3B1", "3A0", "3A1"}},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(
    ToolboxExpandRangesInStringTest, ToolboxExpandRangesInStringTest,
    ::testing::ValuesIn(sToolboxExpandRangesInStringTestData));

/*******************************************************************************
 *  Parametrized floatToString() Tests
 ******************************************************************************/

struct ToolboxFloatToStringTestData {
  double number;
  int decimals;
  QLocale locale;
  QString output;
};

class ToolboxFloatToStringTest
  : public ToolboxTest,
    public ::testing::WithParamInterface<ToolboxFloatToStringTestData> {};

TEST_P(ToolboxFloatToStringTest, test) {
  const ToolboxFloatToStringTestData& data = GetParam();

  QString res = Toolbox::floatToString(data.number, data.decimals, data.locale);
  EXPECT_EQ(data.output.toStdString(), res.toStdString());
}

// clang-format off
static ToolboxFloatToStringTestData
    sToolboxFloatToStringTestData[] = {
// number,      decimals, locale,                 output
  {0.0,         0,        QLocale::c(),           "0"},
  {-2.6,        0,        QLocale::c(),           "-3"},
  {12345.6789,  0,        QLocale::c(),           "12346"},
  {0.0,         1,        QLocale::c(),           "0.0"},
  {-1234.567,   1,        QLocale::c(),           "-1234.6"},
  {1234.567891, 5,        QLocale::c(),           "1234.56789"},
  {0.0,         5,        QLocale("de_DE"),       "0,0"},
  {12345.6789,  5,        QLocale("de_DE"),       "12345,6789"},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(ToolboxFloatToStringTest, ToolboxFloatToStringTest,
                         ::testing::ValuesIn(sToolboxFloatToStringTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
