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
  QString orig_str;
  Length  value;
  QString gen_str;
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
    EXPECT_EQ(Length::fromMm(data.orig_str), data.value);
  } else {
    EXPECT_THROW(Length::fromMm(data.orig_str), RuntimeError);
  }
}

TEST_P(LengthTest, testToMmString) {
  const LengthTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.value.toMmString(), data.gen_str);
  }
}

TEST(LengthTest, testSnapToGrid) {
  EXPECT_EQ(Length(0).mappedToGrid(10), Length(0));
  EXPECT_EQ(Length(10).mappedToGrid(0), Length(10));
  EXPECT_EQ(Length(-10).mappedToGrid(0), Length(-10));
  EXPECT_EQ(Length(10).mappedToGrid(1), Length(10));
  EXPECT_EQ(Length(-10).mappedToGrid(1), Length(-10));
  EXPECT_EQ(Length(8).mappedToGrid(10), Length(10));
  EXPECT_EQ(Length(2).mappedToGrid(10), Length(0));
  EXPECT_EQ(Length(-8).mappedToGrid(10), Length(-10));
  EXPECT_EQ(Length(-2).mappedToGrid(10), Length(0));
  EXPECT_EQ(Length(18).mappedToGrid(10), Length(20));
  EXPECT_EQ(Length(12).mappedToGrid(10), Length(10));
  EXPECT_EQ(Length(-18).mappedToGrid(10), Length(-20));
  EXPECT_EQ(Length(-12).mappedToGrid(10), Length(-10));
  EXPECT_EQ(Length(10).mappedToGrid(10), Length(10));
  EXPECT_EQ(Length(-10).mappedToGrid(10), Length(-10));
  EXPECT_EQ(Length(20).mappedToGrid(20), Length(20));
  EXPECT_EQ(Length(-20).mappedToGrid(20), Length(-20));
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_CASE_P(LengthTest, LengthTest, ::testing::Values(
    LengthTestData({true,  "0",              Length(0),           "0.0"         }),
    LengthTestData({true,  "1",              Length(1000000),     "1.0"         }),
    LengthTestData({true,  "-1",             Length(-1000000),    "-1.0"        }),
    LengthTestData({true,  "0.000001",       Length(1),           "0.000001"    }),
    LengthTestData({true,  "-0.000001",      Length(-1),          "-0.000001"   }),
    LengthTestData({true,  "1e-6",           Length(1),           "0.000001"    }),
    LengthTestData({true,  "-1e-6",          Length(-1),          "-0.000001"   }),
    LengthTestData({true,  "1.000001",       Length(1000001),     "1.000001"    }),
    LengthTestData({true,  "-1.000001",      Length(-1000001),    "-1.000001"   }),
    LengthTestData({true,  "1e3",            Length(1000000000),  "1000.0"      }),
    LengthTestData({true,  "-1e3",           Length(-1000000000), "-1000.0"     }),
    LengthTestData({true,  ".1",             Length(100000),      "0.1"         }),
    LengthTestData({true,  "1.",             Length(1000000),     "1.0"         }),
    LengthTestData({true,  "2147483647e-6",  Length(2147483647),  "2147.483647" }),
    LengthTestData({true,  "-2147483648e-6", Length(-2147483648), "-2147.483648"}),
    LengthTestData({true,  "9",              Length(9000000),     "9.0"         }),
    LengthTestData({true,  "9.9",            Length(9900000),     "9.9"         }),
    LengthTestData({true,  "0.9",            Length(900000),      "0.9"         }),
    LengthTestData({true,  "0.99",           Length(990000),      "0.99"        }),
    LengthTestData({true,  "0.09",           Length(90000),       "0.09"        }),
    LengthTestData({true,  "0.099",          Length(99000),       "0.099"       }),
    LengthTestData({true,  "0.009",          Length(9000),        "0.009"       }),
    LengthTestData({true,  "0.0099",         Length(9900),        "0.0099"      }),
    LengthTestData({true,  "0.0009",         Length(900),         "0.0009"      }),
    LengthTestData({true,  "0.00099",        Length(990),         "0.00099"     }),
    LengthTestData({true,  "0.00009",        Length(90),          "0.00009"     }),
    LengthTestData({true,  "0.000099",       Length(99),          "0.000099"    }),
    LengthTestData({true,  "0.000009",       Length(9),           "0.000009"    }),

    // invalid cases
    LengthTestData({false, "",               Length(),            QString()     }),
    LengthTestData({false, ".",              Length(),            QString()     }),
    LengthTestData({false, "0e",             Length(),            QString()     }),
    LengthTestData({false, "0e+",            Length(),            QString()     }),
    LengthTestData({false, "0e-",            Length(),            QString()     }),
    LengthTestData({false, "0.0000001",      Length(),            QString()     }),
    LengthTestData({false, "1e-7",           Length(),            QString()     }),
    LengthTestData({false, "1e1000",         Length(),            QString()     })
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
