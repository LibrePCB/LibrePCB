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
#include <librepcb/core/geometry/image.h>
#include <librepcb/core/serialization/sexpression.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ImageTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ImageTest, testConstructFromSExpression) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(image b9445237-8982-4a9f-af06-bfc6c507e010 (file \"foo-bar.png\")"
      " (position 17.78 7.62) (rotation 45.0) (width 15.24) (height 5.08)"
      " (border none)"
      ")",
      FilePath());
  Image obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("b9445237-8982-4a9f-af06-bfc6c507e010"),
            obj.getUuid());
  EXPECT_EQ("foo-bar.png", *obj.getFileName());
  EXPECT_EQ(Point(17780000, 7620000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(PositiveLength(15240000), obj.getWidth());
  EXPECT_EQ(PositiveLength(5080000), obj.getHeight());
  EXPECT_EQ(std::nullopt, obj.getBorderWidth());
}

TEST_F(ImageTest, testSerializeAndDeserialize) {
  Image obj1(Uuid::createRandom(), FileProofName("foo.svg"), Point(123, 456),
             Angle::deg45(), PositiveLength(123), PositiveLength(456),
             UnsignedLength(111));
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  Image obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
