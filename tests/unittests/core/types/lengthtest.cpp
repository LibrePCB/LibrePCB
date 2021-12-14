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
#include <librepcb/core/types/length.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  bool valid;
  QString origStr;
  Length value;
  QString genStr;
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
    EXPECT_EQ(Length::fromMm(data.origStr), data.value);
  } else {
    EXPECT_THROW(Length::fromMm(data.origStr), RuntimeError);
  }
}

TEST_P(LengthTest, testToMmString) {
  const LengthTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.value.toMmString(), data.genStr);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(LengthTest, LengthTest, ::testing::Values(
    // from/to mm
    LengthTestData({true,  "0",              Length(0),           "0.0"         }),
    LengthTestData({true,  "1",              Length(1000000),     "1.0"         }),
    LengthTestData({true,  "-1",             Length(-1000000),    "-1.0",       }),
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
 *  Tests for mappedToGrid()
 ******************************************************************************/

typedef struct {
  Length value;
  Length gridInterval;
  Length mappedToGrid;
} LengthMappedToGridData;

class LengthMappedToGrid
  : public ::testing::TestWithParam<LengthMappedToGridData> {};

TEST_P(LengthMappedToGrid, testSnapToGrid) {
  const LengthMappedToGridData& data = GetParam();
  EXPECT_EQ(data.value.mappedToGrid(data.gridInterval), data.mappedToGrid);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(LengthMappedToGrid, LengthMappedToGrid, ::testing::Values(
    LengthMappedToGridData({Length(0),   Length(10), Length(0)  }),
    LengthMappedToGridData({Length(10),  Length(0),  Length(10) }),
    LengthMappedToGridData({Length(-10), Length(0),  Length(-10)}),
    LengthMappedToGridData({Length(10),  Length(1),  Length(10) }),
    LengthMappedToGridData({Length(-10), Length(1),  Length(-10)}),
    LengthMappedToGridData({Length(8),   Length(10), Length(10) }),
    LengthMappedToGridData({Length(2),   Length(10), Length(0)  }),
    LengthMappedToGridData({Length(-8),  Length(10), Length(-10)}),
    LengthMappedToGridData({Length(-2),  Length(10), Length(0)  }),
    LengthMappedToGridData({Length(18),  Length(10), Length(20) }),
    LengthMappedToGridData({Length(12),  Length(10), Length(10) }),
    LengthMappedToGridData({Length(-18), Length(10), Length(-20)}),
    LengthMappedToGridData({Length(-12), Length(10), Length(-10)}),
    LengthMappedToGridData({Length(10),  Length(10), Length(10) }),
    LengthMappedToGridData({Length(-10), Length(10), Length(-10)}),
    LengthMappedToGridData({Length(20),  Length(10), Length(20) }),
    LengthMappedToGridData({Length(-20), Length(10), Length(-20)})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
