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
#include <librepcb/common/units/length.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  Length value;
  Length gridInterval;
  Length mappedToGrid;
} LengthSnapTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class LengthSnapTest : public ::testing::TestWithParam<LengthSnapTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/
TEST_P(LengthSnapTest, testSnapToGrid) {
  const LengthSnapTestData& data = GetParam();
  EXPECT_EQ(data.value.mappedToGrid(data.gridInterval), data.mappedToGrid);
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_CASE_P(LengthSnapTest, LengthSnapTest, ::testing::Values(
    LengthSnapTestData({Length(0),   Length(10), Length(0)  }),
    LengthSnapTestData({Length(10),  Length(0),  Length(10) }),
    LengthSnapTestData({Length(-10), Length(0),  Length(-10)}),
    LengthSnapTestData({Length(10),  Length(1),  Length(10) }),
    LengthSnapTestData({Length(-10), Length(1),  Length(-10)}),
    LengthSnapTestData({Length(8),   Length(10), Length(10) }),
    LengthSnapTestData({Length(2),   Length(10), Length(0)  }),
    LengthSnapTestData({Length(-8),  Length(10), Length(-10)}),
    LengthSnapTestData({Length(-2),  Length(10), Length(0)  }),
    LengthSnapTestData({Length(18),  Length(10), Length(20) }),
    LengthSnapTestData({Length(12),  Length(10), Length(10) }),
    LengthSnapTestData({Length(-18), Length(10), Length(-20)}),
    LengthSnapTestData({Length(-12), Length(10), Length(-10)}),
    LengthSnapTestData({Length(10),  Length(10), Length(10) }),
    LengthSnapTestData({Length(-10), Length(10), Length(-10)}),
    LengthSnapTestData({Length(20),  Length(10), Length(20) }),
    LengthSnapTestData({Length(-20), Length(10), Length(-20)})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
