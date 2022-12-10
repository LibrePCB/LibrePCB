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
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/serialization/sexpression.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class HoleTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(HoleTest, testConstructFromSExpression) {
  SExpression sexpr = SExpression::parse(
      "(hole b9445237-8982-4a9f-af06-bfc6c507e010 (diameter 0.5)"
      " (vertex (position 1.234 2.345) (angle 45.0))"
      ")",
      FilePath());
  Hole obj(sexpr);
  EXPECT_EQ(Uuid::fromString("b9445237-8982-4a9f-af06-bfc6c507e010"),
            obj.getUuid());
  EXPECT_EQ(PositiveLength(500000), obj.getDiameter());
  EXPECT_EQ(1, obj.getPath()->getVertices().count());
  EXPECT_EQ(Point(1234000, 2345000),
            obj.getPath()->getVertices().first().getPos());
  EXPECT_EQ(Angle(45000000), obj.getPath()->getVertices().first().getAngle());
}

TEST_F(HoleTest, testSerializeAndDeserialize) {
  Hole obj1(Uuid::createRandom(), PositiveLength(123),
            NonEmptyPath(Path({Vertex(Point(123, 456), Angle::deg45()),
                               Vertex(Point(789, 321), Angle::deg0())})));
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  Hole obj2(sexpr1);
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
