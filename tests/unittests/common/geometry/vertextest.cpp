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
#include <librepcb/common/geometry/vertex.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class VertexTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(VertexTest, testConstructFromSExpression) {
  SExpression sexpr = SExpression::parse(
      "(vertex (position 1.2 3.4) (angle 45.0))", FilePath());
  Vertex obj(sexpr);
  EXPECT_EQ(Point(1200000, 3400000), obj.getPos());
  EXPECT_EQ(Angle::deg45(), obj.getAngle());
}

TEST_F(VertexTest, testSerializeAndDeserialize) {
  Vertex obj1(Point(123, 567), Angle(789));
  SExpression sexpr1 = obj1.serializeToDomElement("vertex");

  Vertex obj2(sexpr1);
  SExpression sexpr2 = obj2.serializeToDomElement("vertex");

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
