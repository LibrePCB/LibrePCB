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
#include <librepcb/core/types/angle.h>

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
  Angle value;
  QString genStr;
} AngleTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AngleTest : public ::testing::TestWithParam<AngleTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AngleTest, testInverted) {
  EXPECT_EQ(Angle(0).inverted(), Angle(0));
  EXPECT_EQ(Angle(10000000).inverted(), Angle(-350000000));
  EXPECT_EQ(Angle(-350000000).inverted(), Angle(10000000));
  EXPECT_EQ(Angle(180000000).inverted(), Angle(-180000000));
}

TEST_F(AngleTest, testRounded) {
  EXPECT_EQ(Angle(54).rounded(Angle(-1)), Angle(54));  // Invalid -> ignored
  EXPECT_EQ(Angle(54).rounded(Angle(0)), Angle(54));  // Invalid -> ignored
  EXPECT_EQ(Angle(54).rounded(Angle(1)), Angle(54));  // already OK
  EXPECT_EQ(Angle(1000000).rounded(Angle(10)), Angle(1000000));  // already OK
  EXPECT_EQ(Angle(54).rounded(Angle(10)), Angle(50));  // rounded down
  EXPECT_EQ(Angle(55).rounded(Angle(10)), Angle(60));  // rounded up
  EXPECT_EQ(Angle(56).rounded(Angle(10)), Angle(60));  // rounded up
  EXPECT_EQ(Angle(-54).rounded(Angle(10)), Angle(-50));  // rounded down
  EXPECT_EQ(Angle(-55).rounded(Angle(10)), Angle(-60));  // rounded up
  EXPECT_EQ(Angle(-56).rounded(Angle(10)), Angle(-60));  // rounded up
  EXPECT_EQ(Angle(359999990).rounded(Angle(100)), Angle(0));  // overflow
  EXPECT_EQ(Angle(-359999990).rounded(Angle(100)), Angle(0));  // underflow
}

TEST_P(AngleTest, testFromDeg) {
  const AngleTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(Angle::fromDeg(data.origStr), data.value);
  } else {
    EXPECT_THROW(Angle::fromDeg(data.origStr), RuntimeError);
  }
}

TEST_P(AngleTest, testToDegString) {
  const AngleTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.value.toDegString(), data.genStr);
  }
}

TEST_P(AngleTest, testSerialize) {
  const AngleTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.genStr % "\n", serialize(data.value).toByteArray());
  }
}

TEST_P(AngleTest, testDeserializeV01) {
  const AngleTestData& data = GetParam();

  SExpression sexpr = SExpression::createString(data.origStr);
  if (data.valid) {
    EXPECT_EQ(data.value,
              deserialize<Angle>(sexpr, Version::fromString("0.1")));
  } else {
    EXPECT_THROW(deserialize<Angle>(sexpr, Version::fromString("0.1")),
                 RuntimeError);
  }
}

TEST_P(AngleTest, testDeserializeCurrentVersion) {
  const AngleTestData& data = GetParam();

  SExpression sexpr = SExpression::createString(data.origStr);
  if (data.valid) {
    EXPECT_EQ(data.value,
              deserialize<Angle>(sexpr, qApp->getFileFormatVersion()));
  } else {
    EXPECT_THROW(deserialize<Angle>(sexpr, qApp->getFileFormatVersion()),
                 RuntimeError);
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(AngleTest, AngleTest, ::testing::Values(
    // from/to deg
    AngleTestData({true,  "0",          Angle(0),         "0.0"       }),
    AngleTestData({true,  "90",         Angle(90000000),  "90.0"      }),
    AngleTestData({true,  "-90",        Angle(-90000000), "-90.0"     }),
    AngleTestData({true,  "90.000001",  Angle(90000001),  "90.000001" }),
    AngleTestData({true,  "-90.000001", Angle(-90000001), "-90.000001"}),
    AngleTestData({true,  "1e3",        Angle(280000000), "280.0"     }),
    AngleTestData({true,  "0.1",        Angle(100000),    "0.1"       }),

    // invalid cases
    AngleTestData({false, "",           Angle(),          QString()   }),
    AngleTestData({false, ".",          Angle(),          QString()   }),
    AngleTestData({false, "0e",         Angle(),          QString()   }),
    AngleTestData({false, "0e+",        Angle(),          QString()   }),
    AngleTestData({false, "0e-",        Angle(),          QString()   }),
    AngleTestData({false, "0.0000001",  Angle(),          QString()   }),
    AngleTestData({false, "1e-7",       Angle(),          QString()   }),
    AngleTestData({false, "1e1000",     Angle(),          QString()   })
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
