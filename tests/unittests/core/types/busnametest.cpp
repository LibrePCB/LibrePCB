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
#include <librepcb/core/types/busname.h>

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
} BusNameTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class BusNameTest : public ::testing::TestWithParam<BusNameTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(BusNameTest, testConstructor) {
  const BusNameTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *BusName(data.input));
  } else {
    EXPECT_THROW(BusName(data.input), RuntimeError);
  }
}

TEST_P(BusNameTest, testClean) {
  const BusNameTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, cleanBusName(data.input));
  } else {
    QString cleaned = cleanBusName(data.input);
    if (!cleaned.isEmpty()) {
      BusName identifier(cleaned);  // must not throw
    }
  }
}

TEST_P(BusNameTest, testSerialize) {
  const BusNameTestData& data = GetParam();
  if (data.valid) {
    BusName identifier(data.input);
    EXPECT_EQ(data.input, serialize(identifier)->getValue());
  }
}

TEST_P(BusNameTest, testDeserialize) {
  const BusNameTestData& data = GetParam();
  std::unique_ptr<SExpression> sexpr = SExpression::createToken(data.input);
  if (data.valid) {
    EXPECT_EQ(data.input, *deserialize<BusName>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<BusName>(*sexpr), Exception);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(BusNameTest, BusNameTest, ::testing::Values(
    // valid identifiers
    BusNameTestData({"1", true}),
    BusNameTestData({"A", true}),
    BusNameTestData({"z", true}),
    BusNameTestData({"_", true}),
    BusNameTestData({"+", true}),
    BusNameTestData({"-", true}),
    BusNameTestData({"Bus[]", true}),
    BusNameTestData({"DATA[0..7]", true}),
    BusNameTestData({"01234567890123456789012345678901", true}),
    BusNameTestData({"._+-/!?&@#$asDF1234()", true}),

    // invalid identifiers
    BusNameTestData({"", false}), // empty
    BusNameTestData({"012345678901234567890123456789012", false}), // too long
    BusNameTestData({" ", false}), // space
    BusNameTestData({"A B", false}), // space
    BusNameTestData({";", false}), // invalid character
    BusNameTestData({":1234", false}), // invalid character at start
    BusNameTestData({"AS:df", false}), // invalid character in the middle
    BusNameTestData({"1234:", false}), // invalid character at end
    BusNameTestData({"\n", false}), // invalid character
    BusNameTestData({"Foo\tBar", false}), // invalid character in the middle
    BusNameTestData({"Foo\nBar", false}), // invalid character in the middle
    BusNameTestData({"\nFoo", false}), // invalid character at start
    BusNameTestData({"Foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
