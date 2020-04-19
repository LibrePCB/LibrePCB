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
#include <librepcb/common/drillsize.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class DrillSizeTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(DrillSizeTest, testSerialize) {
  DrillSize drillSize =
      DrillSize(PositiveLength(3500000), PositiveLength(5500000));
  SExpression sexpr = SExpression::createList("drill");
  drillSize.serialize(sexpr);
  std::string serialized = sexpr.toByteArray().toStdString();
  ASSERT_EQ(serialized, "(drill 3.5 5.5)\n");
}

TEST(DrillSizeTest, testIsCircular) {
  DrillSize ds1 = DrillSize(PositiveLength(3), PositiveLength(3));
  DrillSize ds2 = DrillSize(PositiveLength(3), PositiveLength(5));
  DrillSize ds3 = DrillSize(PositiveLength(5), PositiveLength(3));
  DrillSize ds4 = DrillSize(PositiveLength(5), PositiveLength(5));
  ASSERT_TRUE(ds1.isCircular());
  ASSERT_FALSE(ds2.isCircular());
  ASSERT_FALSE(ds3.isCircular());
  ASSERT_TRUE(ds4.isCircular());
}

TEST(DrillSizeTest, testCircularConstructor) {
  DrillSize ds = DrillSize(PositiveLength(3));
  ASSERT_EQ(ds.getWidth(), PositiveLength(3));
  ASSERT_EQ(ds.getHeight(), PositiveLength(3));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
