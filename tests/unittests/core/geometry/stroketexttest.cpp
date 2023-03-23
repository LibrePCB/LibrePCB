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
#include <librepcb/core/geometry/stroketext.h>
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

class StrokeTextTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(StrokeTextTest, testConstructFromSExpression) {
  SExpression sexpr = SExpression::parse(
      "(stroke_text 0a8d7180-68e1-4749-bf8c-538b0d88f08c (layer bot_placement) "
      "(height 1.0) (stroke_width 0.2) (letter_spacing auto) "
      "(line_spacing auto) (align left bottom) (position 1.234 2.345) "
      "(rotation 45.0) (auto_rotate true) (mirror true) (value \"Foo Bar\"))",
      FilePath());
  StrokeText obj(sexpr);
  EXPECT_EQ(Uuid::fromString("0a8d7180-68e1-4749-bf8c-538b0d88f08c"),
            obj.getUuid());
  EXPECT_EQ("bot_placement", obj.getLayer().getId());
  EXPECT_EQ(PositiveLength(1000000), obj.getHeight());
  EXPECT_EQ(UnsignedLength(200000), obj.getStrokeWidth());
  EXPECT_EQ(true, obj.getLetterSpacing().isAuto());
  EXPECT_EQ(true, obj.getLineSpacing().isAuto());
  EXPECT_EQ(Alignment(HAlign::left(), VAlign::bottom()), obj.getAlign());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(true, obj.getAutoRotate());
  EXPECT_EQ(true, obj.getMirrored());
  EXPECT_EQ("Foo Bar", obj.getText());
}

TEST_F(StrokeTextTest, testSerializeAndDeserialize) {
  StrokeText obj1(Uuid::createRandom(), Layer::botCopper(), "hello world",
                  Point(12, 34), Angle(56), PositiveLength(123),
                  UnsignedLength(456), StrokeTextSpacing(),
                  StrokeTextSpacing(Ratio(1234)),
                  Alignment(HAlign::right(), VAlign::center()), true, false);
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  StrokeText obj2(sexpr1);
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
