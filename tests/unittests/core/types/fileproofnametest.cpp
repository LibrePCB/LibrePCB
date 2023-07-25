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
#include <librepcb/core/types/fileproofname.h>

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
} FileProofNameTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FileProofNameTest
  : public ::testing::TestWithParam<FileProofNameTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(FileProofNameTest, testConstructor) {
  const FileProofNameTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *FileProofName(data.input));
  } else {
    EXPECT_THROW(FileProofName(data.input), RuntimeError);
  }
}

TEST_P(FileProofNameTest, testClean) {
  const FileProofNameTestData& data = GetParam();

  EXPECT_EQ(data.cleaned, cleanFileProofName(data.input));
}

TEST_P(FileProofNameTest, testSerialize) {
  const FileProofNameTestData& data = GetParam();

  if (data.valid) {
    FileProofName obj(data.input);
    EXPECT_EQ("\"" % data.input % "\"\n", serialize(obj).toByteArray());
  }
}

TEST_P(FileProofNameTest, testDeserialize) {
  const FileProofNameTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(
        FileProofName(data.input),
        deserialize<FileProofName>(SExpression::createString(data.input)));
  } else {
    EXPECT_THROW(
        deserialize<FileProofName>(SExpression::createString(data.input)),
        RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(FileProofNameTest, FileProofNameTest, ::testing::Values(
    // Valid strings.
    FileProofNameTestData({"1", "1", true}),
    FileProofNameTestData({"foo-bar_+().", "foo-bar_+().", true}),

    // Invalid strings.
    FileProofNameTestData({"", "", false}), // too short
    FileProofNameTestData({"123456789012345678901", "12345678901234567890", false}), // too long
    FileProofNameTestData({" ", "", false}), // space
    FileProofNameTestData({"äöü", "aou", false}), // invalid characters
    FileProofNameTestData({" ABC", "ABC", false}), // leading space
    FileProofNameTestData({"ABC ", "ABC", false}), // trailing space
    FileProofNameTestData({"AB CD", "AB-CD", false}), // invalid character
    FileProofNameTestData({"AB\nCD", "ABCD", false}), // invalid character
    FileProofNameTestData({"AB/CD", "ABCD", false}), // invalid character
    FileProofNameTestData({"AB:CD", "ABCD", false}) // invalid character
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
