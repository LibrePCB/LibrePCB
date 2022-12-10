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
#include <librepcb/core/project/board/boardfabricationoutputsettings.h>
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

class BoardFabricationOutputSettingsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(BoardFabricationOutputSettingsTest, testSerializeAndDeserialize) {
  BoardFabricationOutputSettings obj1;
  obj1.setOutputBasePath("a");
  obj1.setSuffixDrills("b");
  obj1.setSuffixDrillsNpth("c");
  obj1.setSuffixDrillsPth("d");
  obj1.setSuffixOutlines("e");
  obj1.setSuffixCopperTop("f");
  obj1.setSuffixCopperInner("g");
  obj1.setSuffixCopperBot("h");
  obj1.setSuffixSolderMaskTop("i");
  obj1.setSuffixSolderMaskBot("j");
  obj1.setSuffixSilkscreenTop("k");
  obj1.setSuffixSilkscreenBot("l");
  obj1.setSuffixSolderPasteTop("m");
  obj1.setSuffixSolderPasteBot("n");
  obj1.setSilkscreenLayersTop({"o", "p"});
  obj1.setSilkscreenLayersBot({"q", "r"});
  obj1.setMergeDrillFiles(!obj1.getMergeDrillFiles());
  obj1.setUseG85SlotCommand(!obj1.getUseG85SlotCommand());
  obj1.setEnableSolderPasteTop(!obj1.getEnableSolderPasteTop());
  obj1.setEnableSolderPasteBot(!obj1.getEnableSolderPasteBot());
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  BoardFabricationOutputSettings obj2(sexpr1);
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
