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
#include <librepcb/core/geometry/via.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/layer.h>

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

TEST_F(ViaTest, testConstructFromSExpression) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(via b9445237-8982-4a9f-af06-bfc6c507e010 (from top_cu) (to in2_cu)"
      " (position 1.234 2.345) (size 0.9) (drill 0.4) (exposure off)"
      ")",
      FilePath());
  Via obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("b9445237-8982-4a9f-af06-bfc6c507e010"),
            obj.getUuid());
  EXPECT_EQ(&Layer::topCopper(), &obj.getStartLayer());
  EXPECT_EQ(Layer::innerCopper(2), &obj.getEndLayer());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(PositiveLength(900000), obj.getSize());
  EXPECT_EQ(PositiveLength(400000), obj.getDrillDiameter());
  EXPECT_EQ(MaskConfig::off(), obj.getExposureConfig());
}

TEST_F(ViaTest, testSerializeAndDeserialize) {
  Via obj1(Uuid::createRandom(), Layer::topCopper(), Layer::botCopper(),
           Point(123, 456), PositiveLength(789), PositiveLength(321),
           MaskConfig::off());
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  Via obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
