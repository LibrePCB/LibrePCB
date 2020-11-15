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
#include <librepcb/common/application.h>
#include <librepcb/project/boards/boardfabricationoutputsettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class BoardFabricationOutputSettingsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(BoardFabricationOutputSettingsTest, testSerializeAndDeserialize) {
  BoardFabricationOutputSettings obj;
  obj.setOutputBasePath("a");
  obj.setSuffixDrills("b");
  obj.setSuffixDrillsNpth("c");
  obj.setSuffixDrillsPth("d");
  obj.setSuffixOutlines("e");
  obj.setSuffixCopperTop("f");
  obj.setSuffixCopperInner("g");
  obj.setSuffixCopperBot("h");
  obj.setSuffixSolderMaskTop("i");
  obj.setSuffixSolderMaskBot("j");
  obj.setSuffixSilkscreenTop("k");
  obj.setSuffixSilkscreenBot("l");
  obj.setSuffixSolderPasteTop("m");
  obj.setSuffixSolderPasteBot("n");
  obj.setSilkscreenLayersTop({"o", "p"});
  obj.setSilkscreenLayersBot({"q", "r"});
  obj.setMergeDrillFiles(!obj.getMergeDrillFiles());
  obj.setEnableSolderPasteTop(!obj.getEnableSolderPasteTop());
  obj.setEnableSolderPasteBot(!obj.getEnableSolderPasteBot());
  SExpression sexpr1 = obj.serializeToDomElement("settings");

  BoardFabricationOutputSettings obj2(sexpr1, qApp->getFileFormatVersion());
  SExpression sexpr2 = obj2.serializeToDomElement("settings");

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace project
}  // namespace librepcb
