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
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/serialization/sexpression.h>

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

TEST_F(BoardDesignRulesTest, testConstructFromSExpression) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(design_rules\n"
      " (default_trace_width 0.31)\n"
      " (stopmask_max_via_drill_diameter 0.2)\n"
      " (stopmask_clearance (ratio 0.1) (min 1.1) (max 2.1))\n"
      " (solderpaste_clearance (ratio 0.3) (min 1.3) (max 2.3))\n"
      " (pad_annular_ring (outer auto) (inner full)"
      "  (ratio 0.4) (min 1.4) (max 2.4))\n"
      " (via_annular_ring (ratio 0.5) (min 1.5) (max 2.5))\n"
      ")",
      FilePath());
  BoardDesignRules obj(*sexpr);
  EXPECT_EQ(PositiveLength(310000), obj.getDefaultTraceWidth());
  EXPECT_EQ(UnsignedLength(200000), obj.getStopMaskMaxViaDiameter());
  EXPECT_EQ(UnsignedRatio(Ratio(100000)),
            obj.getStopMaskClearance().getRatio());
  EXPECT_EQ(UnsignedLength(1100000), obj.getStopMaskClearance().getMinValue());
  EXPECT_EQ(UnsignedLength(2100000), obj.getStopMaskClearance().getMaxValue());
  EXPECT_EQ(UnsignedRatio(Ratio(300000)),
            obj.getSolderPasteClearance().getRatio());
  EXPECT_EQ(UnsignedLength(1300000),
            obj.getSolderPasteClearance().getMinValue());
  EXPECT_EQ(UnsignedLength(2300000),
            obj.getSolderPasteClearance().getMaxValue());
  EXPECT_EQ(true, obj.getPadCmpSideAutoAnnularRing());
  EXPECT_EQ(false, obj.getPadInnerAutoAnnularRing());
  EXPECT_EQ(UnsignedRatio(Ratio(400000)), obj.getPadAnnularRing().getRatio());
  EXPECT_EQ(UnsignedLength(1400000), obj.getPadAnnularRing().getMinValue());
  EXPECT_EQ(UnsignedLength(2400000), obj.getPadAnnularRing().getMaxValue());
  EXPECT_EQ(UnsignedRatio(Ratio(500000)), obj.getViaAnnularRing().getRatio());
  EXPECT_EQ(UnsignedLength(1500000), obj.getViaAnnularRing().getMinValue());
  EXPECT_EQ(UnsignedLength(2500000), obj.getViaAnnularRing().getMaxValue());
}

TEST_F(BoardDesignRulesTest, testSerializeAndDeserialize) {
  BoardDesignRules obj1;
  obj1.setDefaultTraceWidth(PositiveLength(33));
  obj1.setStopMaskMaxViaDiameter(UnsignedLength(44));
  obj1.setStopMaskClearance(BoundedUnsignedRatio(
      UnsignedRatio(Ratio(11)), UnsignedLength(22), UnsignedLength(33)));
  obj1.setSolderPasteClearance(BoundedUnsignedRatio(
      UnsignedRatio(Ratio(55)), UnsignedLength(66), UnsignedLength(77)));
  obj1.setPadCmpSideAutoAnnularRing(true);
  obj1.setPadInnerAutoAnnularRing(false);
  obj1.setPadAnnularRing(BoundedUnsignedRatio(
      UnsignedRatio(Ratio(88)), UnsignedLength(99), UnsignedLength(111)));
  obj1.setViaAnnularRing(BoundedUnsignedRatio(
      UnsignedRatio(Ratio(222)), UnsignedLength(333), UnsignedLength(444)));
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  BoardDesignRules obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
