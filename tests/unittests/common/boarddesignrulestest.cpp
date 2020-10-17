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
#include <librepcb/common/boarddesignrules.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class BoardDesignRulesTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(BoardDesignRulesTest, testSerializeAndDeserialize) {
  BoardDesignRules obj1;
  obj1.setName(ElementName("foo bar"));
  obj1.setDescription("Foo Bar");
  obj1.setStopMaskClearanceRatio(UnsignedRatio(Ratio(11)));
  obj1.setStopMaskClearanceBounds(UnsignedLength(22), UnsignedLength(33));
  obj1.setStopMaskMaxViaDiameter(UnsignedLength(44));
  obj1.setCreamMaskClearanceRatio(UnsignedRatio(Ratio(55)));
  obj1.setCreamMaskClearanceBounds(UnsignedLength(66), UnsignedLength(77));
  obj1.setRestringPadRatio(UnsignedRatio(Ratio(88)));
  obj1.setRestringPadBounds(UnsignedLength(99), UnsignedLength(111));
  obj1.setRestringViaRatio(UnsignedRatio(Ratio(222)));
  obj1.setRestringViaBounds(UnsignedLength(333), UnsignedLength(444));
  SExpression sexpr1 = obj1.serializeToDomElement("rules");

  BoardDesignRules obj2(sexpr1);
  SExpression sexpr2 = obj2.serializeToDomElement("rules");

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
