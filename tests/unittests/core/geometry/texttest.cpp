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
#include <librepcb/core/geometry/text.h>
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

class TextTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(TextTest, testConstructFromSExpression) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(text eabf43fb-496b-4dc8-8ff7-ffac67991390 (layer sym_names) "
      "(value \"{{NAME}}\") (align center bottom) (height 2.54) "
      "(position 1.234 2.345) (rotation 45.0) (lock true))",
      FilePath());
  Text obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("eabf43fb-496b-4dc8-8ff7-ffac67991390"),
            obj.getUuid());
  EXPECT_EQ("sym_names", obj.getLayer().getId());
  EXPECT_EQ("{{NAME}}", obj.getText());
  EXPECT_EQ(Alignment(HAlign::center(), VAlign::bottom()), obj.getAlign());
  EXPECT_EQ(PositiveLength(2540000), obj.getHeight());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(true, obj.isLocked());
}

TEST_F(TextTest, testSerializeAndDeserialize) {
  Text obj1(Uuid::createRandom(), Layer::botCopper(), "foo bar", Point(12, 34),
            Angle(56), PositiveLength(78),
            Alignment(HAlign::right(), VAlign::center()), false);
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  Text obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
