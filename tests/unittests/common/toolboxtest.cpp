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
 *  Test Methods
 ******************************************************************************/

TEST(ToolboxTest, testStringOrNumberToQVariant_numeric) {
  QString  string  = "1337";
  QVariant variant = Toolbox::stringOrNumberToQVariant(string);
  EXPECT_EQ(QVariant::Int, variant.type());
  EXPECT_EQ(QVariant(1337), variant);
}

TEST(ToolboxTest, testStringOrNumberToQVariant_nonNumeric) {
  QString  string  = "leet";
  QVariant variant = Toolbox::stringOrNumberToQVariant(string);
  EXPECT_EQ(QVariant::String, variant.type());
  EXPECT_EQ(QVariant("leet"), variant);
}

TEST(ToolboxTest, testStringOrNumberToQVariant_mixed) {
  QString  string  = "l33t";
  QVariant variant = Toolbox::stringOrNumberToQVariant(string);
  EXPECT_EQ(QVariant::String, variant.type());
  EXPECT_EQ(QVariant("l33t"), variant);
}

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

INSTANTIATE_TEST_CASE_P(
    ToolboxIncrementNumberInStringTest, ToolboxIncrementNumberInStringTest,
    ::testing::ValuesIn(sToolboxIncrementNumberInStringTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
