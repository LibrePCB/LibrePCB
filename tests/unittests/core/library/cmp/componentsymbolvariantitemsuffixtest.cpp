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
#include <librepcb/core/library/cmp/componentsymbolvariantitemsuffix.h>

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
} ComponentSymbolVariantItemSuffixTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ComponentSymbolVariantItemSuffixTest
  : public ::testing::TestWithParam<ComponentSymbolVariantItemSuffixTestData> {
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(ComponentSymbolVariantItemSuffixTest, testConstructor) {
  const ComponentSymbolVariantItemSuffixTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *ComponentSymbolVariantItemSuffix(data.input));
  } else {
    EXPECT_THROW(ComponentSymbolVariantItemSuffix(data.input), RuntimeError);
  }
}

TEST_P(ComponentSymbolVariantItemSuffixTest, testClean) {
  const ComponentSymbolVariantItemSuffixTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, cleanComponentSymbolVariantItemSuffix(data.input));
  } else {
    QString cleaned = cleanComponentSymbolVariantItemSuffix(data.input);
    ComponentSymbolVariantItemSuffix suffix(cleaned);  // must not throw
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(ComponentSymbolVariantItemSuffixTest,ComponentSymbolVariantItemSuffixTest, ::testing::Values(
    // valid keys
    ComponentSymbolVariantItemSuffixTestData({"", true}),
    ComponentSymbolVariantItemSuffixTestData({"1", true}),
    ComponentSymbolVariantItemSuffixTestData({"A", true}),
    ComponentSymbolVariantItemSuffixTestData({"z", true}),
    ComponentSymbolVariantItemSuffixTestData({"_", true}),
    ComponentSymbolVariantItemSuffixTestData({"_a_B_C_", true}),
    ComponentSymbolVariantItemSuffixTestData({"0123456789012345", true}),

    // invalid keys
    ComponentSymbolVariantItemSuffixTestData({"01234567890123456", false}), // too long
    ComponentSymbolVariantItemSuffixTestData({" ", false}), // space
    ComponentSymbolVariantItemSuffixTestData({"A B", false}), // space
    ComponentSymbolVariantItemSuffixTestData({";", false}), // invalid character
    ComponentSymbolVariantItemSuffixTestData({":1234", false}), // invalid character at start
    ComponentSymbolVariantItemSuffixTestData({"AS:df", false}), // invalid character in the middle
    ComponentSymbolVariantItemSuffixTestData({"1234:", false}), // invalid character at end
    ComponentSymbolVariantItemSuffixTestData({"\n", false}), // invalid character
    ComponentSymbolVariantItemSuffixTestData({"Foo\tBar", false}), // invalid character in the middle
    ComponentSymbolVariantItemSuffixTestData({"Foo\nBar", false}), // invalid character in the middle
    ComponentSymbolVariantItemSuffixTestData({"\nFoo", false}), // invalid character at start
    ComponentSymbolVariantItemSuffixTestData({"Foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
