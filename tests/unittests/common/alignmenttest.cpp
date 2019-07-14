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
#include <librepcb/common/alignment.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  bool   valid;
  HAlign origHAling;
  VAlign origVAling;
  HAlign mirrHAling;
  VAlign mirrVAling;
} AlignmentTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AlignmentTest : public ::testing::TestWithParam<AlignmentTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/
TEST_P(AlignmentTest, testMirror) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    alignment.mirror();
    EXPECT_EQ(alignment, Alignment(data.mirrHAling, data.mirrVAling));
  }
}

TEST_P(AlignmentTest, testMirrorH) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    alignment.mirrorH();
    EXPECT_EQ(alignment, Alignment(data.mirrHAling, data.origVAling));
  }
}

TEST_P(AlignmentTest, testMirrorV) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    alignment.mirrorV();
    EXPECT_EQ(alignment, Alignment(data.origHAling, data.mirrVAling));
  }
}

TEST_P(AlignmentTest, testMirrored) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    Alignment alignmentMirrored = alignment.mirrored();
    EXPECT_EQ(alignment, Alignment(data.origHAling, data.origVAling));
    EXPECT_EQ(alignmentMirrored, Alignment(data.mirrHAling, data.mirrVAling));
  }
}

TEST_P(AlignmentTest, testMirroredH) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    Alignment alignmentMirroredH = alignment.mirroredH();
    EXPECT_EQ(alignment, Alignment(data.origHAling, data.origVAling));
    EXPECT_EQ(alignmentMirroredH, Alignment(data.mirrHAling, data.origVAling));
  }
}

TEST_P(AlignmentTest, testMirroredV) {
  const AlignmentTestData& data = GetParam();

  if (data.valid) {
    Alignment alignment = Alignment(data.origHAling, data.origVAling);
    Alignment alignmentMirroredV = alignment.mirroredV();
    EXPECT_EQ(alignment, Alignment(data.origHAling, data.origVAling));
    EXPECT_EQ(alignmentMirroredV, Alignment(data.origHAling, data.mirrVAling));
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(AlignmentTest, AlignmentTest, ::testing::Values(
    // mirrored
    AlignmentTestData({true, HAlign::left(),   VAlign::bottom(),
                       HAlign::right(),  VAlign::top()}),
    AlignmentTestData({true, HAlign::right(),  VAlign::top(),
                       HAlign::left(),   VAlign::bottom()}),
    AlignmentTestData({true, HAlign::center(), VAlign::center(),
                       HAlign::center(), VAlign::center()})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
