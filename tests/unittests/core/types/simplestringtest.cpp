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
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/simplestring.h>

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
  QString cleaned;
  bool valid;
} SimpleStringTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SimpleStringTest : public ::testing::TestWithParam<SimpleStringTestData> {
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(SimpleStringTest, testConstructor) {
  const SimpleStringTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *SimpleString(data.input));
  } else {
    EXPECT_THROW(SimpleString(data.input), RuntimeError);
  }
}

TEST_P(SimpleStringTest, testClean) {
  const SimpleStringTestData& data = GetParam();

  EXPECT_EQ(data.cleaned, cleanSimpleString(data.input));
}

TEST_P(SimpleStringTest, testSerialize) {
  const SimpleStringTestData& data = GetParam();

  if (data.valid) {
    SimpleString obj(data.input);
    EXPECT_EQ("\"" % data.input % "\"\n", serialize(obj)->toByteArray());
  }
}

TEST_P(SimpleStringTest, testDeserialize) {
  const SimpleStringTestData& data = GetParam();

  const std::unique_ptr<const SExpression> sexpr =
      SExpression::createString(data.input);
  if (data.valid) {
    EXPECT_EQ(SimpleString(data.input), deserialize<SimpleString>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<SimpleString>(*sexpr), RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(SimpleStringTest, SimpleStringTest, ::testing::Values(
    // Valid strings.
    SimpleStringTestData({"", "", true}),
    SimpleStringTestData({"1", "1", true}),
    SimpleStringTestData({"foo:_-+*ç%&/()=", "foo:_-+*ç%&/()=", true}),
    SimpleStringTestData({"_", "_", true}),
    SimpleStringTestData({"_A B  C", "_A B C", true}),

    // Invalid strings.
    SimpleStringTestData({" ABC", "ABC", false}), // leading space
    SimpleStringTestData({"ABC ", "ABC", false}), // trailing space
    SimpleStringTestData({"AB\n\nCD", "AB CD", false}), // invalid character
    SimpleStringTestData({"AB\r\rCD", "AB CD", false}), // invalid character
    SimpleStringTestData({"AB\t\tCD", "AB CD", false}) // invalid character
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
