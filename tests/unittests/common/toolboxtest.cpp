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
#include <librepcb/common/toolbox.h>

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
  QString     input;
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
  QDebug  dbg(&msg);
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
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
