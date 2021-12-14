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
#include <librepcb/core/graphics/graphicslayername.h>

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
} GraphicsLayerNameTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GraphicsLayerNameTest
  : public ::testing::TestWithParam<GraphicsLayerNameTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(GraphicsLayerNameTest, testConstructor) {
  const GraphicsLayerNameTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *GraphicsLayerName(data.input));
  } else {
    EXPECT_THROW(GraphicsLayerName(data.input), RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(GraphicsLayerNameTest, GraphicsLayerNameTest, ::testing::Values(
    // valid keys
    GraphicsLayerNameTestData({"a", true}),
    GraphicsLayerNameTestData({"a1", true}),
    GraphicsLayerNameTestData({"a_b_c_1_2_3", true}),
    GraphicsLayerNameTestData({"abcdefghijklmnopqrstuvwabcdefghijklmnopq", true}),

    // invalid keys
    GraphicsLayerNameTestData({"", false}), // empty
    GraphicsLayerNameTestData({"abcdefghijklmnopqrstuvwabcdefghijklmnopqr", false}), // too long
    GraphicsLayerNameTestData({" ", false}), // space
    GraphicsLayerNameTestData({"1a", false}), // digit at start
    GraphicsLayerNameTestData({"_a", false}), // underscore at start
    GraphicsLayerNameTestData({"A", false}), // upperspace letter
    GraphicsLayerNameTestData({"a b", false}), // space
    GraphicsLayerNameTestData({";", false}), // invalid character
    GraphicsLayerNameTestData({":abcd", false}), // invalid character at start
    GraphicsLayerNameTestData({"as:df", false}), // invalid character in the middle
    GraphicsLayerNameTestData({"abcd:", false}), // invalid character at end
    GraphicsLayerNameTestData({"\n", false}), // invalid character
    GraphicsLayerNameTestData({"foo\tbar", false}), // invalid character in the middle
    GraphicsLayerNameTestData({"foo\nbar", false}), // invalid character in the middle
    GraphicsLayerNameTestData({"\nfoo", false}), // invalid character at start
    GraphicsLayerNameTestData({"foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
