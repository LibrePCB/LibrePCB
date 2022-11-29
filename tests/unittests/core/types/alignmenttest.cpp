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
#include <librepcb/core/application.h>
#include <librepcb/core/types/alignment.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  HAlign hAlign;
  VAlign vAlign;
  HAlign hMirrored;
  VAlign vMirrored;
  QByteArray serialized;
  bool validSExpression;
} AlignmentTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AlignmentTest : public ::testing::TestWithParam<AlignmentTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(AlignmentTest, testConstructFromSExpressionV01) {
  const AlignmentTestData& data = GetParam();

  SExpression sexpr = SExpression::parse(data.serialized, FilePath());

  if (data.validSExpression) {
    EXPECT_EQ(Alignment(data.hAlign, data.vAlign),
              Alignment(sexpr, Version::fromString("0.1")));
  } else {
    EXPECT_THROW({ Alignment a(sexpr, Version::fromString("0.1")); },
                 RuntimeError);
  }
}

TEST_P(AlignmentTest, testConstructFromSExpressionCurrentVersion) {
  const AlignmentTestData& data = GetParam();

  SExpression sexpr = SExpression::parse(data.serialized, FilePath());

  if (data.validSExpression) {
    EXPECT_EQ(Alignment(data.hAlign, data.vAlign),
              Alignment(sexpr, qApp->getFileFormatVersion()));
  } else {
    EXPECT_THROW({ Alignment a(sexpr, qApp->getFileFormatVersion()); },
                 RuntimeError);
  }
}

TEST_P(AlignmentTest, testSerialize) {
  const AlignmentTestData& data = GetParam();

  if (data.validSExpression) {
    Alignment alignment(data.hAlign, data.vAlign);
    SExpression sexpr = SExpression::createList("align");
    alignment.serialize(sexpr);
    EXPECT_EQ(data.serialized, sexpr.toByteArray());
  }
}

TEST_P(AlignmentTest, testMirror) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  alignment.mirror();
  EXPECT_EQ(alignment, Alignment(data.hMirrored, data.vMirrored));
}

TEST_P(AlignmentTest, testMirrorH) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  alignment.mirrorH();
  EXPECT_EQ(alignment, Alignment(data.hMirrored, data.vAlign));
}

TEST_P(AlignmentTest, testMirrorV) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  alignment.mirrorV();
  EXPECT_EQ(alignment, Alignment(data.hAlign, data.vMirrored));
}

TEST_P(AlignmentTest, testMirrored) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  Alignment alignmentMirrored = alignment.mirrored();
  EXPECT_EQ(alignment, Alignment(data.hAlign, data.vAlign));
  EXPECT_EQ(alignmentMirrored, Alignment(data.hMirrored, data.vMirrored));
}

TEST_P(AlignmentTest, testMirroredH) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  Alignment alignmentMirroredH = alignment.mirroredH();
  EXPECT_EQ(alignment, Alignment(data.hAlign, data.vAlign));
  EXPECT_EQ(alignmentMirroredH, Alignment(data.hMirrored, data.vAlign));
}

TEST_P(AlignmentTest, testMirroredV) {
  const AlignmentTestData& data = GetParam();

  Alignment alignment = Alignment(data.hAlign, data.vAlign);
  Alignment alignmentMirroredV = alignment.mirroredV();
  EXPECT_EQ(alignment, Alignment(data.hAlign, data.vAlign));
  EXPECT_EQ(alignmentMirroredV, Alignment(data.hAlign, data.vMirrored));
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(AlignmentTest, AlignmentTest, ::testing::Values(
  // Invalid serialization
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "(align \"\" \"\")\n", false}),
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "(align center foo)\n", false}),
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "(align center)\n", false}),
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "(align)\n", false}),
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "center\n", false}),
  // Valid serialization
  AlignmentTestData({HAlign::left(),   VAlign::bottom(),
                     HAlign::right(),  VAlign::top(),
                     "(align left bottom)\n", true}),
  AlignmentTestData({HAlign::right(),  VAlign::top(),
                     HAlign::left(),   VAlign::bottom(),
                     "(align right top)\n", true}),
  AlignmentTestData({HAlign::center(), VAlign::center(),
                     HAlign::center(), VAlign::center(),
                     "(align center center)\n", true})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
