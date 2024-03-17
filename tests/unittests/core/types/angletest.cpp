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
#include <librepcb/core/exceptions.h>
#include <librepcb/core/serialization/sexpression.h>
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
    EXPECT_EQ(data.genStr % "\n", serialize(data.value)->toByteArray());
  }
}

TEST_P(AngleTest, testDeserialize) {
  const AngleTestData& data = GetParam();

  std::unique_ptr<SExpression> sexpr = SExpression::createString(data.origStr);
  if (data.valid) {
    EXPECT_EQ(data.value, deserialize<Angle>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<Angle>(*sexpr), RuntimeError);
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
 *  Parametrized setAngleDeg(float) and setAngleRad() Tests
 ******************************************************************************/

struct AngleSetAngleFloatTestData {
  qreal inputDegrees;
  qreal inputRadians;
  qint32 output;  // Microdegrees
  bool overflow;
};

class AngleSetAngleFloatTest
  : public ::testing::TestWithParam<AngleSetAngleFloatTestData> {};

TEST_P(AngleSetAngleFloatTest, testDeg) {
  const AngleSetAngleFloatTestData& data = GetParam();

  Angle a;
  a.setAngleDeg(data.inputDegrees);
  EXPECT_EQ(data.output, a.toMicroDeg());
  EXPECT_EQ(data.output, Angle::fromDeg(data.inputDegrees).toMicroDeg());
  if (!data.overflow) {
    EXPECT_NEAR(data.inputDegrees, a.toDeg(), 1e-6);
  }
}

TEST_P(AngleSetAngleFloatTest, testRad) {
  const AngleSetAngleFloatTestData& data = GetParam();

  Angle a;
  a.setAngleRad(data.inputRadians);
  EXPECT_EQ(data.output, a.toMicroDeg());
  EXPECT_EQ(data.output, Angle::fromRad(data.inputRadians).toMicroDeg());
  if (!data.overflow) {
    EXPECT_NEAR(data.inputRadians, a.toRad(), 1e-7);
  }
}

static AngleSetAngleFloatTestData sSetAngleFloatTestData[] = {
    // {degrees, radians, microdegrees, overflow}
    {0.0, 0.0, 0, false},
    {-0.0, 0.0, 0, false},
    {180.123456, 3.143747367, 180123456, false},
    {-180.123456, -3.143747367, -180123456, false},
    {359.999999, 6.28318529, 359999999, false},
    {-359.999999, -6.28318529, -359999999, false},
    {360.0, 6.2831853072, 0, true},  // overflow
    {-360.0, -6.2831853072, 0, true},  // underflow
    {360.1, 6.2849306364, 100000, true},  // overflow
    {-360.1, -6.2849306364, -100000, true},  // underflow
    {359.9999999, 6.2831853054, 0, true},  // round -> overflow
    {-359.9999999, -6.2831853054, 0, true},  // round -> overflow
    {360.0000006, 6.2831853177, 1, true},  // round -> overflow
    {-360.0000006, -6.2831853177, -1, true},  // round -> underflow
    {0.1000004, 0.0017453362, 100000, true},  // round
    {-0.1000004, -0.0017453362, -100000, true},  // round
    {0.1000006, 0.0017453397, 100001, true},  // round
    {-0.1000006, -0.0017453397, -100001, true},  // round
};

INSTANTIATE_TEST_SUITE_P(AngleSetAngleFloatTest, AngleSetAngleFloatTest,
                         ::testing::ValuesIn(sSetAngleFloatTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
