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
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FootprintPadTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(FootprintPadTest, testConstructFromSExpressionConnected) {
  SExpression sexpr = SExpression::parse(
      "(pad 7040952d-7016-49cd-8c3e-6078ecca98b9 (side top) (shape rect)\n"
      " (position 1.234 2.345) (rotation 45.0) (size 1.1 2.2) (drill 0.0)\n"
      " (package_pad d48b8bd2-a46c-4495-87a5-662747034098)\n"
      ")",
      FilePath());
  FootprintPad obj(sexpr);
  EXPECT_EQ(Uuid::fromString("7040952d-7016-49cd-8c3e-6078ecca98b9"),
            obj.getUuid());
  EXPECT_EQ(Uuid::fromString("d48b8bd2-a46c-4495-87a5-662747034098"),
            obj.getPackagePadUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(FootprintPad::Shape::RECT, obj.getShape());
  EXPECT_EQ(PositiveLength(1100000), obj.getWidth());
  EXPECT_EQ(UnsignedLength(2200000), obj.getHeight());
  EXPECT_EQ(UnsignedLength(0), obj.getDrillDiameter());
  EXPECT_EQ(FootprintPad::BoardSide::TOP, obj.getBoardSide());
}

TEST_F(FootprintPadTest, testConstructFromSExpressionUnconnected) {
  SExpression sexpr = SExpression::parse(
      "(pad 7040952d-7016-49cd-8c3e-6078ecca98b9 (side top) (shape rect)\n"
      " (position 1.234 2.345) (rotation 45.0) (size 1.1 2.2) (drill 1.0)\n"
      " (package_pad none)\n"
      ")",
      FilePath());
  FootprintPad obj(sexpr);
  EXPECT_EQ(Uuid::fromString("7040952d-7016-49cd-8c3e-6078ecca98b9"),
            obj.getUuid());
  EXPECT_EQ(tl::nullopt, obj.getPackagePadUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(FootprintPad::Shape::RECT, obj.getShape());
  EXPECT_EQ(PositiveLength(1100000), obj.getWidth());
  EXPECT_EQ(UnsignedLength(2200000), obj.getHeight());
  EXPECT_EQ(UnsignedLength(1000000), obj.getDrillDiameter());
  EXPECT_EQ(FootprintPad::BoardSide::TOP, obj.getBoardSide());
}

TEST_F(FootprintPadTest, testSerializeAndDeserialize) {
  FootprintPad obj1(Uuid::createRandom(), Uuid::createRandom(), Point(123, 567),
                    Angle(789), FootprintPad::Shape::OCTAGON,
                    PositiveLength(123), PositiveLength(456),
                    UnsignedLength(100000), FootprintPad::BoardSide::THT);
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  FootprintPad obj2(sexpr1);
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
