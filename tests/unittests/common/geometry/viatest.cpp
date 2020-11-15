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
#include <librepcb/common/application.h>
#include <librepcb/common/geometry/via.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ViaTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ViaTest, testConstructFromSExpressionV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression sexpr = SExpression::parse(
      "(via b9445237-8982-4a9f-af06-bfc6c507e010 (position 1.234 2.345) "
      "(size 0.9) (drill 0.4) (shape round))",
      FilePath());
  Via obj(sexpr, Version::fromString("0.1"));
  EXPECT_EQ(Uuid::fromString("b9445237-8982-4a9f-af06-bfc6c507e010"),
            obj.getUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(PositiveLength(900000), obj.getSize());
  EXPECT_EQ(PositiveLength(400000), obj.getDrillDiameter());
  EXPECT_EQ(Via::Shape::Round, obj.getShape());
}

TEST_F(ViaTest, testConstructFromSExpressionCurrentVersion) {
  SExpression sexpr = SExpression::parse(
      "(via b9445237-8982-4a9f-af06-bfc6c507e010 (position 1.234 2.345) "
      "(size 0.9) (drill 0.4) (shape round))",
      FilePath());
  Via obj(sexpr, qApp->getFileFormatVersion());
  EXPECT_EQ(Uuid::fromString("b9445237-8982-4a9f-af06-bfc6c507e010"),
            obj.getUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(PositiveLength(900000), obj.getSize());
  EXPECT_EQ(PositiveLength(400000), obj.getDrillDiameter());
  EXPECT_EQ(Via::Shape::Round, obj.getShape());
}

TEST_F(ViaTest, testSerializeAndDeserialize) {
  Via obj1(Uuid::createRandom(), Point(123, 456), Via::Shape::Octagon,
           PositiveLength(789), PositiveLength(321));
  SExpression sexpr1 = obj1.serializeToDomElement("via");

  Via obj2(sexpr1, qApp->getFileFormatVersion());
  SExpression sexpr2 = obj2.serializeToDomElement("via");

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
