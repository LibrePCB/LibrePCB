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
#include <librepcb/core/library/cmp/componentprefix.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  QString input;
  bool valid;
} ComponentPrefixTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ComponentPrefixTest
  : public ::testing::TestWithParam<ComponentPrefixTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(ComponentPrefixTest, testConstructor) {
  const ComponentPrefixTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *ComponentPrefix(data.input));
  } else {
    EXPECT_THROW(ComponentPrefix(data.input), RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(ComponentPrefixTest, ComponentPrefixTest, ::testing::Values(
    // valid keys
    ComponentPrefixTestData({"", true}),
    ComponentPrefixTestData({"A", true}),
    ComponentPrefixTestData({"z", true}),
    ComponentPrefixTestData({"_", true}),
    ComponentPrefixTestData({"_a_B_C_", true}),
    ComponentPrefixTestData({"abcdefghijklmnop", true}),

    // invalid keys
    ComponentPrefixTestData({"abcdefghijklmnopq", false}), // too long
    ComponentPrefixTestData({" ", false}), // space
    ComponentPrefixTestData({"A1", false}), // digit
    ComponentPrefixTestData({"A B", false}), // space
    ComponentPrefixTestData({";", false}), // invalid character
    ComponentPrefixTestData({":abcd", false}), // invalid character at start
    ComponentPrefixTestData({"AS:df", false}), // invalid character in the middle
    ComponentPrefixTestData({"abcd:", false}), // invalid character at end
    ComponentPrefixTestData({"\n", false}), // invalid character
    ComponentPrefixTestData({"Foo\tBar", false}), // invalid character in the middle
    ComponentPrefixTestData({"Foo\nBar", false}), // invalid character in the middle
    ComponentPrefixTestData({"\nFoo", false}), // invalid character at start
    ComponentPrefixTestData({"Foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
