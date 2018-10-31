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
  bool    valid;
  QString str;
  Length  value;
} LengthTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class LengthTest : public ::testing::TestWithParam<LengthTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/
TEST_P(LengthTest, testFromMm) {
  const LengthTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(Length::fromMm(data.str), data.value);
  } else {
    EXPECT_THROW(Length::fromMm(data.str), RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_CASE_P(LengthTest, LengthTest, ::testing::Values(
    // DCE Version 4 (random, the only accepted UUID type for us)
    LengthTestData({true,  "0",              Length(0)          }),
    LengthTestData({true,  "1",              Length(1000000)    }),
    LengthTestData({true,  "-1",             Length(-1000000)   }),
    LengthTestData({true,  "0.000001",       Length(1)          }),
    LengthTestData({true,  "-0.000001",      Length(-1)         }),
    LengthTestData({true,  "1e-6",           Length(1)          }),
    LengthTestData({true,  "-1e-6",          Length(-1)         }),
    LengthTestData({true,  "1.000001",       Length(1000001)    }),
    LengthTestData({true,  "-1.000001",      Length(-1000001)   }),
    LengthTestData({true,  "1e3",            Length(1000000000) }),
    LengthTestData({true,  "-1e3",           Length(-1000000000)}),
    LengthTestData({true,  ".1",             Length(100000)     }),
    LengthTestData({true,  "1.",             Length(1000000)    }),
    LengthTestData({true,  "2147483647e-6",  Length(2147483647) }),
    LengthTestData({true,  "-2147483648e-6", Length(-2147483648)}),
    // invalid cases
    LengthTestData({false, "",               Length()           }),
    LengthTestData({false, ".",              Length()           }),
    LengthTestData({false, "0e",             Length()           }),
    LengthTestData({false, "0e+",            Length()           }),
    LengthTestData({false, "0e-",            Length()           }),
    LengthTestData({false, "0.0000001",      Length()           }),
    LengthTestData({false, "1e-7",           Length()           }),
    LengthTestData({false, "1e1000",         Length()           })

));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
