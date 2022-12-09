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
#include <librepcb/core/attribute/attributekey.h>
#include <librepcb/core/serialization/sexpression.h>

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
} AttributeKeyTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AttributeKeyTest : public ::testing::TestWithParam<AttributeKeyTestData> {
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(AttributeKeyTest, testConstructor) {
  const AttributeKeyTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *AttributeKey(data.input));
  } else {
    EXPECT_THROW(AttributeKey(data.input), RuntimeError);
  }
}

TEST_P(AttributeKeyTest, testClean) {
  const AttributeKeyTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, cleanAttributeKey(data.input));
  } else {
    QString cleaned = cleanAttributeKey(data.input);
    if (!cleaned.isEmpty()) {
      AttributeKey key(cleaned);  // must not throw
    }
  }
}

TEST_P(AttributeKeyTest, testSerialize) {
  const AttributeKeyTestData& data = GetParam();

  if (data.valid) {
    AttributeKey obj(data.input);
    EXPECT_EQ("\"" % data.input % "\"\n", serialize(obj).toByteArray());
  }
}

TEST_P(AttributeKeyTest, testDeserialize) {
  const AttributeKeyTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(AttributeKey(data.input),
              deserialize<AttributeKey>(SExpression::createString(data.input)));
  } else {
    EXPECT_THROW(
        deserialize<AttributeKey>(SExpression::createString(data.input)),
        RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(AttributeKeyTest, AttributeKeyTest, ::testing::Values(
    // valid keys
    AttributeKeyTestData({"1", true}),
    AttributeKeyTestData({"A", true}),
    AttributeKeyTestData({"_", true}),
    AttributeKeyTestData({"_A_2_C_", true}),
    AttributeKeyTestData({"0123456789012345678901234567890123456789", true}),

    // invalid keys
    AttributeKeyTestData({"", false}), // empty
    AttributeKeyTestData({"01234567890123456789012345678901234567890", false}), // too long
    AttributeKeyTestData({" ", false}), // space
    AttributeKeyTestData({"A B", false}), // space
    AttributeKeyTestData({"z", false}), // lowercase character
    AttributeKeyTestData({";", false}), // invalid character
    AttributeKeyTestData({":1234", false}), // invalid character at start
    AttributeKeyTestData({"AS:DF", false}), // invalid character in the middle
    AttributeKeyTestData({"1234:", false}), // invalid character at end
    AttributeKeyTestData({"\n", false}), // invalid character
    AttributeKeyTestData({"FOO\tBAR", false}), // invalid character in the middle
    AttributeKeyTestData({"FOO\nBAR", false}), // invalid character in the middle
    AttributeKeyTestData({"\nFOO", false}), // invalid character at start
    AttributeKeyTestData({"FOO\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
