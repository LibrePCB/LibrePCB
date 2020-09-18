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
#include "boardfabricationoutputsettings.h"

#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardFabricationOutputSettings::BoardFabricationOutputSettings() noexcept
  : mOutputBasePath("./output/{{VERSION}}/gerber/{{PROJECT}}"),
    mSuffixDrills("_DRILLS.drl"),
    mSuffixDrillsNpth("_DRILLS-NPTH.drl"),
    mSuffixDrillsPth("_DRILLS-PTH.drl"),
    mSuffixOutlines("_OUTLINES.gbr"),
    mSuffixCopperTop("_COPPER-TOP.gbr"),
    mSuffixCopperInner("_COPPER-IN{{CU_LAYER}}.gbr"),
    mSuffixCopperBot("_COPPER-BOTTOM.gbr"),
    mSuffixSolderMaskTop("_SOLDERMASK-TOP.gbr"),
    mSuffixSolderMaskBot("_SOLDERMASK-BOTTOM.gbr"),
    mSuffixSilkscreenTop("_SILKSCREEN-TOP.gbr"),
    mSuffixSilkscreenBot("_SILKSCREEN-BOTTOM.gbr"),
    mSuffixSolderPasteTop("_SOLDERPASTE-TOP.gbr"),
    mSuffixSolderPasteBot("_SOLDERPASTE-BOTTOM.gbr"),
    mSilkscreenLayersTop(
        {GraphicsLayer::sTopPlacement, GraphicsLayer::sTopNames}),
    mSilkscreenLayersBot(
        {GraphicsLayer::sBotPlacement, GraphicsLayer::sBotNames}),
    mMergeDrillFiles(false),
    mEnableSolderPasteTop(false),
    mEnableSolderPasteBot(false) {
}

BoardFabricationOutputSettings::BoardFabricationOutputSettings(
    const BoardFabricationOutputSettings& other) noexcept
  : BoardFabricationOutputSettings()  // init and load defaults
{
  *this = other;
}

BoardFabricationOutputSettings::BoardFabricationOutputSettings(
    const SExpression& node)
  : BoardFabricationOutputSettings()  // init and load defaults
{
  mOutputBasePath = node.getValueByPath<QString>("base_path");
  mSuffixOutlines = node.getValueByPath<QString>("outlines/suffix");
  mSuffixCopperTop = node.getValueByPath<QString>("copper_top/suffix");
  mSuffixCopperInner = node.getValueByPath<QString>("copper_inner/suffix");
  mSuffixCopperBot = node.getValueByPath<QString>("copper_bot/suffix");
  mSuffixSolderMaskTop = node.getValueByPath<QString>("soldermask_top/suffix");
  mSuffixSolderMaskBot = node.getValueByPath<QString>("soldermask_bot/suffix");
  mSuffixSilkscreenTop = node.getValueByPath<QString>("silkscreen_top/suffix");
  mSuffixSilkscreenBot = node.getValueByPath<QString>("silkscreen_bot/suffix");
  mSuffixSolderPasteTop =
      node.getValueByPath<QString>("solderpaste_top/suffix");
  mSuffixSolderPasteBot =
      node.getValueByPath<QString>("solderpaste_bot/suffix");
  mSuffixDrillsPth = node.getValueByPath<QString>("drills/suffix_pth");
  mSuffixDrillsNpth = node.getValueByPath<QString>("drills/suffix_npth");
  mSuffixDrills = node.getValueByPath<QString>("drills/suffix_merged");
  mMergeDrillFiles = node.getValueByPath<bool>("drills/merge");
  mEnableSolderPasteTop = node.getValueByPath<bool>("solderpaste_top/create");
  mEnableSolderPasteBot = node.getValueByPath<bool>("solderpaste_bot/create");

  mSilkscreenLayersTop.clear();
  foreach (const SExpression& child,
           node.getChildByPath("silkscreen_top/layers").getChildren()) {
    mSilkscreenLayersTop.append(child.getValue<QString>());
  }

  mSilkscreenLayersBot.clear();
  foreach (const SExpression& child,
           node.getChildByPath("silkscreen_bot/layers").getChildren()) {
    mSilkscreenLayersBot.append(child.getValue<QString>());
  }
}

BoardFabricationOutputSettings::~BoardFabricationOutputSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardFabricationOutputSettings::serialize(SExpression& root) const {
  root.appendChild("base_path", mOutputBasePath, true);
  root.appendList("outlines", true)
      .appendChild("suffix", mSuffixOutlines, false);
  root.appendList("copper_top", true)
      .appendChild("suffix", mSuffixCopperTop, false);
  root.appendList("copper_inner", true)
      .appendChild("suffix", mSuffixCopperInner, false);
  root.appendList("copper_bot", true)
      .appendChild("suffix", mSuffixCopperBot, false);
  root.appendList("soldermask_top", true)
      .appendChild("suffix", mSuffixSolderMaskTop, false);
  root.appendList("soldermask_bot", true)
      .appendChild("suffix", mSuffixSolderMaskBot, false);

  SExpression& silkscreenTop = root.appendList("silkscreen_top", true);
  silkscreenTop.appendChild("suffix", mSuffixSilkscreenTop, false);
  SExpression& silkscreenTopLayers = silkscreenTop.appendList("layers", true);
  foreach (const QString& layer, mSilkscreenLayersTop) {
    silkscreenTopLayers.appendChild(SExpression::createToken(layer));
  }

  SExpression& silkscreenBot = root.appendList("silkscreen_bot", true);
  silkscreenBot.appendChild("suffix", mSuffixSilkscreenBot, false);
  SExpression& silkscreenBotLayers = silkscreenBot.appendList("layers", true);
  foreach (const QString& layer, mSilkscreenLayersBot) {
    silkscreenBotLayers.appendChild(SExpression::createToken(layer));
  }

  SExpression& drills = root.appendList("drills", true);
  drills.appendChild("merge", mMergeDrillFiles, false);
  drills.appendChild("suffix_pth", mSuffixDrillsPth, true);
  drills.appendChild("suffix_npth", mSuffixDrillsNpth, true);
  drills.appendChild("suffix_merged", mSuffixDrills, true);

  SExpression& solderPasteTop = root.appendList("solderpaste_top", true);
  solderPasteTop.appendChild("create", mEnableSolderPasteTop, false);
  solderPasteTop.appendChild("suffix", mSuffixSolderPasteTop, false);

  SExpression& solderPasteBot = root.appendList("solderpaste_bot", true);
  solderPasteBot.appendChild("create", mEnableSolderPasteBot, false);
  solderPasteBot.appendChild("suffix", mSuffixSolderPasteBot, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoardFabricationOutputSettings& BoardFabricationOutputSettings::operator=(
    const BoardFabricationOutputSettings& rhs) noexcept {
  mOutputBasePath = rhs.mOutputBasePath;
  mSuffixDrills = rhs.mSuffixDrills;
  mSuffixDrillsNpth = rhs.mSuffixDrillsNpth;
  mSuffixDrillsPth = rhs.mSuffixDrillsPth;
  mSuffixOutlines = rhs.mSuffixOutlines;
  mSuffixCopperTop = rhs.mSuffixCopperTop;
  mSuffixCopperInner = rhs.mSuffixCopperInner;
  mSuffixCopperBot = rhs.mSuffixCopperBot;
  mSuffixSolderMaskTop = rhs.mSuffixSolderMaskTop;
  mSuffixSolderMaskBot = rhs.mSuffixSolderMaskBot;
  mSuffixSilkscreenTop = rhs.mSuffixSilkscreenTop;
  mSuffixSilkscreenBot = rhs.mSuffixSilkscreenBot;
  mSuffixSolderPasteTop = rhs.mSuffixSolderPasteTop;
  mSuffixSolderPasteBot = rhs.mSuffixSolderPasteBot;
  mSilkscreenLayersTop = rhs.mSilkscreenLayersTop;
  mSilkscreenLayersBot = rhs.mSilkscreenLayersBot;
  mMergeDrillFiles = rhs.mMergeDrillFiles;
  mEnableSolderPasteTop = rhs.mEnableSolderPasteTop;
  mEnableSolderPasteBot = rhs.mEnableSolderPasteBot;
  return *this;
}

bool BoardFabricationOutputSettings::operator==(
    const BoardFabricationOutputSettings& rhs) const noexcept {
  if (mOutputBasePath != rhs.mOutputBasePath) return false;
  if (mSuffixDrills != rhs.mSuffixDrills) return false;
  if (mSuffixDrillsNpth != rhs.mSuffixDrillsNpth) return false;
  if (mSuffixDrillsPth != rhs.mSuffixDrillsPth) return false;
  if (mSuffixOutlines != rhs.mSuffixOutlines) return false;
  if (mSuffixCopperTop != rhs.mSuffixCopperTop) return false;
  if (mSuffixCopperInner != rhs.mSuffixCopperInner) return false;
  if (mSuffixCopperBot != rhs.mSuffixCopperBot) return false;
  if (mSuffixSolderMaskTop != rhs.mSuffixSolderMaskTop) return false;
  if (mSuffixSolderMaskBot != rhs.mSuffixSolderMaskBot) return false;
  if (mSuffixSilkscreenTop != rhs.mSuffixSilkscreenTop) return false;
  if (mSuffixSilkscreenBot != rhs.mSuffixSilkscreenBot) return false;
  if (mSuffixSolderPasteTop != rhs.mSuffixSolderPasteTop) return false;
  if (mSuffixSolderPasteBot != rhs.mSuffixSolderPasteBot) return false;
  if (mSilkscreenLayersTop != rhs.mSilkscreenLayersTop) return false;
  if (mSilkscreenLayersBot != rhs.mSilkscreenLayersBot) return false;
  if (mMergeDrillFiles != rhs.mMergeDrillFiles) return false;
  if (mEnableSolderPasteTop != rhs.mEnableSolderPasteTop) return false;
  if (mEnableSolderPasteBot != rhs.mEnableSolderPasteBot) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
