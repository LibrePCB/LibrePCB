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
 *  Test Class
 ******************************************************************************/

class LengthTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/
TEST(LengthTest, testFromMm) {
  EXPECT_EQ(Length::fromMm("0"), Length(0));
  EXPECT_EQ(Length::fromMm("1"), Length(1000000));
  EXPECT_EQ(Length::fromMm("-1"), Length(-1000000));
  EXPECT_EQ(Length::fromMm("0.000001"), Length(1));
  EXPECT_EQ(Length::fromMm("-0.000001"), Length(-1));
  EXPECT_EQ(Length::fromMm("1e-6"), Length(1));
  EXPECT_EQ(Length::fromMm("-1e-6"), Length(-1));
  EXPECT_EQ(Length::fromMm("1.000001"), Length(1000001));
  EXPECT_EQ(Length::fromMm("-1.000001"), Length(-1000001));
  EXPECT_EQ(Length::fromMm("1e3"), Length(1000000000));
  EXPECT_EQ(Length::fromMm("-1e3"), Length(-1000000000));
  EXPECT_EQ(Length::fromMm(".1"), Length(100000));
  EXPECT_EQ(Length::fromMm("1."), Length(1000000));
  EXPECT_EQ(Length::fromMm("2147483647e-6"), Length(2147483647));
  EXPECT_EQ(Length::fromMm("-2147483648e-6"), Length(-2147483648));

  EXPECT_THROW(Length::fromMm("."), RuntimeError);
  EXPECT_THROW(Length::fromMm("0e"), RuntimeError);
  EXPECT_THROW(Length::fromMm("0e+"), RuntimeError);
  EXPECT_THROW(Length::fromMm("0e-"), RuntimeError);
  EXPECT_THROW(Length::fromMm("0e-"), RuntimeError);
  EXPECT_THROW(Length::fromMm("0.0000001"), RuntimeError);
  EXPECT_THROW(Length::fromMm("1e-7"), RuntimeError);
  EXPECT_THROW(Length::fromMm("1e1000"), RuntimeError);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
