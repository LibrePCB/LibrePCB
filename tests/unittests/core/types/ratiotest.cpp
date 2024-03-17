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
#include <librepcb/core/types/ratio.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  Ratio ratio;
  qint32 ppm;
  qreal percent;
  qreal normalized;
  QString string;
} RatioTestData_t;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class RatioTest : public ::testing::TestWithParam<RatioTestData_t> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(RatioTest, testDefaultConstructor) {
  Ratio r;
  EXPECT_EQ(0, r.toPpm());
}

TEST_P(RatioTest, testCopyConstructor) {
  const RatioTestData_t& data = GetParam();

  Ratio r(data.ratio);
  EXPECT_EQ(data.ppm, r.toPpm());
}

TEST_P(RatioTest, testPpmConstructor) {
  const RatioTestData_t& data = GetParam();

  Ratio r(data.ppm);
  EXPECT_EQ(data.ppm, r.toPpm());
}

TEST_P(RatioTest, testSetRatioPpm) {
  const RatioTestData_t& data = GetParam();

  Ratio r;
  r.setRatioPpm(data.ppm);
  EXPECT_EQ(data.ppm, r.toPpm());
}

TEST(RatioTest, testSetRatioPercentInt) {
  Ratio r;
  r.setRatioPercent(42);
  EXPECT_EQ(420000, r.toPpm());
}

TEST_P(RatioTest, testSetRatioPercentFloat) {
  const RatioTestData_t& data = GetParam();

  Ratio r;
  r.setRatioPercent(data.percent);
  EXPECT_NEAR(data.ppm, r.toPpm(), 2);
}

TEST_P(RatioTest, testSetRatioNormalizedFloat) {
  const RatioTestData_t& data = GetParam();

  Ratio r;
  r.setRatioNormalized(data.normalized);
  EXPECT_NEAR(data.ppm, r.toPpm(), 2);
}

TEST_P(RatioTest, testSetRatioNormalizedString) {
  const RatioTestData_t& data = GetParam();

  Ratio r;
  r.setRatioNormalized(data.string);
  EXPECT_EQ(data.ppm, r.toPpm());
}

TEST_P(RatioTest, testToPpm) {
  const RatioTestData_t& data = GetParam();
  EXPECT_EQ(data.ppm, data.ratio.toPpm());
}

TEST_P(RatioTest, testToPercent) {
  const RatioTestData_t& data = GetParam();
  EXPECT_NEAR(data.percent, data.ratio.toPercent(), 0.0002);
}

TEST_P(RatioTest, testToNormalized) {
  const RatioTestData_t& data = GetParam();
  EXPECT_NEAR(data.normalized, data.ratio.toNormalized(), 0.000002);
}

TEST_P(RatioTest, testToNormalizedString) {
  const RatioTestData_t& data = GetParam();
  EXPECT_EQ(data.string, data.ratio.toNormalizedString());
}

TEST_P(RatioTest, testFromPercent) {
  const RatioTestData_t& data = GetParam();
  EXPECT_NEAR(data.ppm, Ratio::fromPercent(data.percent).toPpm(), 2);
}

TEST_P(RatioTest, testFromNormalizedFloat) {
  const RatioTestData_t& data = GetParam();
  EXPECT_NEAR(data.ppm, Ratio::fromNormalized(data.normalized).toPpm(), 2);
}

TEST_P(RatioTest, testFromNormalizedString) {
  const RatioTestData_t& data = GetParam();
  EXPECT_EQ(data.ppm, Ratio::fromNormalized(data.string).toPpm());
}

TEST(RatioTest, testFromPercentInt) {
  EXPECT_EQ(0, Ratio::fromPercent(0).toPpm());
  EXPECT_EQ(500000, Ratio::fromPercent(50).toPpm());
  EXPECT_EQ(1000000, Ratio::fromPercent(100).toPpm());
}

TEST(RatioTest, testFromPercentFloat) {
  EXPECT_NEAR(0.0, Ratio::fromPercent(0.0).toPercent(), 0.0002);
  EXPECT_NEAR(50.0, Ratio::fromPercent(50.0).toPercent(), 0.0002);
  EXPECT_NEAR(100.0, Ratio::fromPercent(100.0).toPercent(), 0.0002);
  EXPECT_NEAR(42.42, Ratio::fromPercent(42.42).toPercent(), 0.0002);
}

TEST_P(RatioTest, testOperatorAssign) {
  const RatioTestData_t& data = GetParam();

  Ratio r;
  r = data.ratio;
  EXPECT_EQ(data.ppm, r.toPpm());
}

TEST(RatioTest, testOperatorEqual) {
  EXPECT_TRUE(Ratio() == Ratio());
  EXPECT_TRUE(Ratio() == Ratio(0));
  EXPECT_TRUE(Ratio(0) == Ratio(0));
  EXPECT_TRUE(Ratio(1234) == Ratio(1234));
  EXPECT_TRUE(Ratio(-987654321) == Ratio(-987654321));
  EXPECT_FALSE(Ratio(0) == Ratio(1));
  EXPECT_FALSE(Ratio(5) == Ratio(-6));
  EXPECT_FALSE(Ratio(-987654321) == Ratio(-987654322));
}

TEST(RatioTest, testOperatorNotEqual) {
  EXPECT_FALSE(Ratio() != Ratio());
  EXPECT_FALSE(Ratio() != Ratio(0));
  EXPECT_FALSE(Ratio(0) != Ratio(0));
  EXPECT_FALSE(Ratio(1234) != Ratio(1234));
  EXPECT_FALSE(Ratio(-987654321) != Ratio(-987654321));
  EXPECT_TRUE(Ratio(0) != Ratio(1));
  EXPECT_TRUE(Ratio(5) != Ratio(-6));
  EXPECT_TRUE(Ratio(-987654321) != Ratio(-987654322));
}

TEST(RatioTest, testOperatorBool) {
  EXPECT_FALSE(Ratio());
  EXPECT_FALSE(Ratio(0));
  EXPECT_TRUE(Ratio(1));
  EXPECT_TRUE(Ratio(1234));
  EXPECT_TRUE(Ratio(-987654321));
}

TEST_P(RatioTest, testSerialize) {
  const RatioTestData_t& data = GetParam();

  EXPECT_EQ(data.string % "\n", serialize(data.ratio)->toByteArray());
}

TEST_P(RatioTest, testDeserialize) {
  const RatioTestData_t& data = GetParam();

  std::unique_ptr<SExpression> sexpr = SExpression::createString(data.string);
  EXPECT_EQ(data.ratio, deserialize<Ratio>(*sexpr));
}

TEST(RatioTest, testDeserializeEmpty) {
  std::unique_ptr<SExpression> sexpr = SExpression::createString("");
  EXPECT_THROW(deserialize<Ratio>(*sexpr), RuntimeError);
}

TEST(RatioTest, testDeserializeInvalid) {
  std::unique_ptr<SExpression> sexpr = SExpression::createString("foo");
  EXPECT_THROW(deserialize<Ratio>(*sexpr), RuntimeError);
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(RatioTest, RatioTest, ::testing::Values(
    //              {            ratio,        ppm,     percent,  normalized,        string}
    RatioTestData_t({Ratio(         0),          0,         0.0,         0.0,    "0.0"     }),
    RatioTestData_t({Ratio(    500000),     500000,        50.0,         0.5,    "0.5"     }),
    RatioTestData_t({Ratio(   1000000),    1000000,       100.0,         1.0,    "1.0"     }),
    RatioTestData_t({Ratio( 123456789),  123456789,  12345.6789,  123.456789,  "123.456789"}),
    RatioTestData_t({Ratio(-987654321), -987654321, -98765.4321, -987.654321, "-987.654321"})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
