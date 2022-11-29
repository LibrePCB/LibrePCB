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
#include <librepcb/core/application.h>
#include <librepcb/core/geometry/text.h>

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

TEST_F(TextTest, testConstructFromSExpressionV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression sexpr = SExpression::parse(
      "(text eabf43fb-496b-4dc8-8ff7-ffac67991390 (layer sym_names) "
      "(value \"{{NAME}}\") (align center bottom) (height 2.54) "
      "(position 1.234 2.345) (rotation 45.0))",
      FilePath());
  Text obj(sexpr, Version::fromString("0.1"));
  EXPECT_EQ(Uuid::fromString("eabf43fb-496b-4dc8-8ff7-ffac67991390"),
            obj.getUuid());
  EXPECT_EQ(GraphicsLayerName("sym_names"), obj.getLayerName());
  EXPECT_EQ("{{NAME}}", obj.getText());
  EXPECT_EQ(Alignment(HAlign::center(), VAlign::bottom()), obj.getAlign());
  EXPECT_EQ(PositiveLength(2540000), obj.getHeight());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
}

TEST_F(TextTest, testConstructFromSExpressionCurrentVersion) {
  SExpression sexpr = SExpression::parse(
      "(text eabf43fb-496b-4dc8-8ff7-ffac67991390 (layer sym_names) "
      "(value \"{{NAME}}\") (align center bottom) (height 2.54) "
      "(position 1.234 2.345) (rotation 45.0))",
      FilePath());
  Text obj(sexpr, qApp->getFileFormatVersion());
  EXPECT_EQ(Uuid::fromString("eabf43fb-496b-4dc8-8ff7-ffac67991390"),
            obj.getUuid());
  EXPECT_EQ(GraphicsLayerName("sym_names"), obj.getLayerName());
  EXPECT_EQ("{{NAME}}", obj.getText());
  EXPECT_EQ(Alignment(HAlign::center(), VAlign::bottom()), obj.getAlign());
  EXPECT_EQ(PositiveLength(2540000), obj.getHeight());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
}

TEST_F(TextTest, testSerializeAndDeserialize) {
  Text obj1(Uuid::createRandom(), GraphicsLayerName("foo"), "foo bar",
            Point(12, 34), Angle(56), PositiveLength(78),
            Alignment(HAlign::right(), VAlign::center()));
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  Text obj2(sexpr1, qApp->getFileFormatVersion());
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
