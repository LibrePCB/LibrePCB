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
#include <librepcb/core/types/tag.h>

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
} TagTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TagTest : public ::testing::TestWithParam<TagTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(TagTest, testConstructor) {
  const TagTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *Tag(data.input));
  } else {
    EXPECT_THROW(Tag(data.input), RuntimeError);
  }
}

TEST_P(TagTest, testClean) {
  const TagTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, cleanTag(data.input));
  } else {
    QString cleaned = cleanTag(data.input);
    if (!cleaned.isEmpty()) {
      Tag tag(cleaned);  // must not throw
    }
  }
}

TEST_P(TagTest, testParse) {
  const TagTestData& data = GetParam();

  const std::optional<Tag> ret = parseTag(data.input);
  ASSERT_EQ(data.valid, ret.has_value());
  if (data.valid) {
    EXPECT_EQ(data.input, **ret);
  }
}

TEST_P(TagTest, testSerialize) {
  const TagTestData& data = GetParam();
  if (data.valid) {
    Tag tag(data.input);
    EXPECT_EQ(data.input, serialize(tag)->getValue());
  }
}

TEST_P(TagTest, testDeserialize) {
  const TagTestData& data = GetParam();
  std::unique_ptr<SExpression> sexpr = SExpression::createToken(data.input);
  if (data.valid) {
    EXPECT_EQ(data.input, *deserialize<Tag>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<Tag>(*sexpr), Exception);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(TagTest, TagTest, ::testing::Values(
    // valid tags
    TagTestData({"1", true}),
    TagTestData({"z", true}),
    TagTestData({"-foo-bar-12.34-", true}),
    TagTestData({"ipc-density-level-a", true}),
    TagTestData({"01234567890123456789012345678901", true}),

    // invalid tags
    TagTestData({"", false}), // empty
    TagTestData({"012345678901234567890123456789012", false}), // too long
    TagTestData({"Z", false}), // uppercase letter
    TagTestData({" ", false}), // space
    TagTestData({"a b", false}), // space
    TagTestData({"~", false}), // invalid character
    TagTestData({":1234", false}), // invalid character at start
    TagTestData({"as:df", false}), // invalid character in the middle
    TagTestData({"1234:", false}), // invalid character at end
    TagTestData({"\n", false}), // invalid character
    TagTestData({"\nfoo", false}), // invalid character at start
    TagTestData({"foo\tbar", false}), // invalid character in the middle
    TagTestData({"foo\nbar", false}), // invalid character in the middle
    TagTestData({"foo bar", false}), // invalid character in the middle
    TagTestData({"foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
