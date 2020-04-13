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
#include <librepcb/common/utils/mathparser.h>
#include <optional/tl/optional.hpp>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  QString             locale;
  QString             input;
  tl::optional<qreal> output;
} MathParserTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class MathParserTest : public ::testing::TestWithParam<MathParserTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/
TEST_P(MathParserTest, test) {
  const MathParserTestData& data = GetParam();

  MathParser parser;
  parser.setLocale(QLocale(data.locale));
  MathParser::Result result = parser.parse(data.input);
  if (data.output) {
    EXPECT_TRUE(result.valid);
    EXPECT_EQ("", result.error);
    EXPECT_EQ(*data.output, result.value);
  } else {
    EXPECT_FALSE(result.valid);
    EXPECT_NE("", result.error);
    EXPECT_EQ(0, result.value);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(MathParserTest, MathParserTest, ::testing::Values(
    // valid cases
    MathParserTestData({"en_US", "0", 0}),
    MathParserTestData({"en_US", "0.1234", 0.1234}),
    MathParserTestData({"en_US", "+0.1234", 0.1234}),
    MathParserTestData({"en_US", "-0.1234", -0.1234}),
    MathParserTestData({"en_US", "2+3", 5}),
    MathParserTestData({"en_US", "(1+2)/2", 1.5}),
    MathParserTestData({"en_US", " 2 * (1.1 + 2.2) / 3.3 ", 2*(1.1+2.2)/3.3}),
    MathParserTestData({"en_US", "5,000", 5000}), // thousand separator
    MathParserTestData({"de_DE", "5,000", 5}), // decimal point

    // invalid cases
    MathParserTestData({"en_US", "", tl::nullopt}),
    MathParserTestData({"en_US", " ", tl::nullopt}),
    MathParserTestData({"en_US", ".", tl::nullopt}),
    MathParserTestData({"en_US", "/", tl::nullopt}),
    MathParserTestData({"en_US", "(1+2", tl::nullopt})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
