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
#include <librepcb/core/utils/overlinemarkupparser.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Tests for extract()
 ******************************************************************************/

typedef struct {
  QString input;
  QString output;
  QVector<std::pair<int, int>> spans;
} ExtractTestData;

class OverlineMarkupParserTest
  : public ::testing::TestWithParam<ExtractTestData> {};

TEST_P(OverlineMarkupParserTest, test) {
  const ExtractTestData& data = GetParam();

  QString output = "foo";
  const QVector<std::pair<int, int>> result =
      OverlineMarkupParser::extract(data.input, output);
  EXPECT_EQ(data.output.toStdString(), output.toStdString());
  EXPECT_EQ(data.spans, result);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(OverlineMarkupParserTest, OverlineMarkupParserTest, ::testing::Values(
  // Without modification at all.
  ExtractTestData{"", "", {}},
  ExtractTestData{"!", "!", {}},
  ExtractTestData{"!!", "!!", {}},
  ExtractTestData{"!!!", "!!!", {}},
  ExtractTestData{"A", "A", {}},
  ExtractTestData{"A/B/C", "A/B/C", {}},
  ExtractTestData{"AB_CD!", "AB_CD!", {}},
  // With substitutions, but without overlines.
  ExtractTestData{"!!A", "!A", {}},
  ExtractTestData{"!!!!A", "!!A", {}},
  ExtractTestData{"A!!B", "A!B", {}},
  ExtractTestData{"A!!!!B", "A!!B", {}},
  ExtractTestData{"!!/!!A", "!/!A", {}},
  ExtractTestData{"!!!!/A", "!!/A", {}},
  // Only with overlines.
  ExtractTestData{"!ABCD", "ABCD", {std::make_pair(0, 4)}},
  ExtractTestData{"AB!CD", "ABCD", {std::make_pair(2, 2)}},
  ExtractTestData{"AB!CD!EF", "ABCDEF", {std::make_pair(2, 2)}},
  ExtractTestData{"!AB/CD", "AB/CD", {std::make_pair(0, 2)}},
  ExtractTestData{"!AB!/CD", "AB/CD", {std::make_pair(0, 5)}},
  ExtractTestData{"AB!/CD", "AB/CD", {std::make_pair(2, 3)}},
  ExtractTestData{"!AB/!CD", "AB/CD", {std::make_pair(0, 2), std::make_pair(3, 2)}},
  ExtractTestData{"!AB/CD/!EF", "AB/CD/EF", {std::make_pair(0, 2), std::make_pair(6, 2)}},
  ExtractTestData{"AB/!CD/EF", "AB/CD/EF", {std::make_pair(3, 2)}},
  ExtractTestData{"!AB!/CD/EF", "AB/CD/EF", {std::make_pair(0, 5)}},
  // Overlines mixed with substitutions.
  ExtractTestData{"!!!AB!!CD", "!AB!CD", {std::make_pair(1, 5)}},
  ExtractTestData{"AB!!!!!CD", "AB!!CD", {std::make_pair(4, 2)}},
  ExtractTestData{"AB!CD!!EF!", "ABCD!EF!", {std::make_pair(2, 6)}},
  ExtractTestData{"!AB!CD!!EF!", "ABCD!EF!", {std::make_pair(0, 2)}},
  ExtractTestData{"!AB/CD!!EF!", "AB/CD!EF!", {std::make_pair(0, 2)}},
  ExtractTestData{"!AB!!/CD", "AB!/CD", {std::make_pair(0, 3)}},
  ExtractTestData{"!AB!!!/CD", "AB!/CD", {std::make_pair(0, 6)}}
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
