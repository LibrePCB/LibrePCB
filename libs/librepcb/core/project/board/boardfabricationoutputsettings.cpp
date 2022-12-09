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

#include "../../graphics/graphicslayer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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
  : mOutputBasePath(node.getChild("base_path/@0").getValue()),
    mSuffixDrills(node.getChild("drills/suffix_merged/@0").getValue()),
    mSuffixDrillsNpth(node.getChild("drills/suffix_npth/@0").getValue()),
    mSuffixDrillsPth(node.getChild("drills/suffix_pth/@0").getValue()),
    mSuffixOutlines(node.getChild("outlines/suffix/@0").getValue()),
    mSuffixCopperTop(node.getChild("copper_top/suffix/@0").getValue()),
    mSuffixCopperInner(node.getChild("copper_inner/suffix/@0").getValue()),
    mSuffixCopperBot(node.getChild("copper_bot/suffix/@0").getValue()),
    mSuffixSolderMaskTop(node.getChild("soldermask_top/suffix/@0").getValue()),
    mSuffixSolderMaskBot(node.getChild("soldermask_bot/suffix/@0").getValue()),
    mSuffixSilkscreenTop(node.getChild("silkscreen_top/suffix/@0").getValue()),
    mSuffixSilkscreenBot(node.getChild("silkscreen_bot/suffix/@0").getValue()),
    mSuffixSolderPasteTop(
        node.getChild("solderpaste_top/suffix/@0").getValue()),
    mSuffixSolderPasteBot(
        node.getChild("solderpaste_bot/suffix/@0").getValue()),
    mSilkscreenLayersTop(),  // Initialized below.
    mSilkscreenLayersBot(),  // Initialized below.
    mMergeDrillFiles(deserialize<bool>(node.getChild("drills/merge/@0"))),
    mEnableSolderPasteTop(
        deserialize<bool>(node.getChild("solderpaste_top/create/@0"))),
    mEnableSolderPasteBot(
        deserialize<bool>(node.getChild("solderpaste_bot/create/@0"))) {
  foreach (const SExpression* child,
           node.getChild("silkscreen_top/layers")
               .getChildren(SExpression::Type::Token)) {
    mSilkscreenLayersTop.append(child->getValue());
  }
  foreach (const SExpression* child,
           node.getChild("silkscreen_bot/layers")
               .getChildren(SExpression::Type::Token)) {
    mSilkscreenLayersBot.append(child->getValue());
  }
}

BoardFabricationOutputSettings::~BoardFabricationOutputSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardFabricationOutputSettings::serialize(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("base_path", mOutputBasePath);
  root.ensureLineBreak();
  root.appendList("outlines").appendChild("suffix", mSuffixOutlines);
  root.ensureLineBreak();
  root.appendList("copper_top").appendChild("suffix", mSuffixCopperTop);
  root.ensureLineBreak();
  root.appendList("copper_inner").appendChild("suffix", mSuffixCopperInner);
  root.ensureLineBreak();
  root.appendList("copper_bot").appendChild("suffix", mSuffixCopperBot);
  root.ensureLineBreak();
  root.appendList("soldermask_top").appendChild("suffix", mSuffixSolderMaskTop);
  root.ensureLineBreak();
  root.appendList("soldermask_bot").appendChild("suffix", mSuffixSolderMaskBot);
  root.ensureLineBreak();

  SExpression& silkscreenTop = root.appendList("silkscreen_top");
  silkscreenTop.appendChild("suffix", mSuffixSilkscreenTop);
  silkscreenTop.ensureLineBreak();
  SExpression& silkscreenTopLayers = silkscreenTop.appendList("layers");
  foreach (const QString& layer, mSilkscreenLayersTop) {
    silkscreenTopLayers.appendChild(SExpression::createToken(layer));
  }
  silkscreenTop.ensureLineBreak();
  root.ensureLineBreak();

  SExpression& silkscreenBot = root.appendList("silkscreen_bot");
  silkscreenBot.appendChild("suffix", mSuffixSilkscreenBot);
  silkscreenBot.ensureLineBreak();
  SExpression& silkscreenBotLayers = silkscreenBot.appendList("layers");
  foreach (const QString& layer, mSilkscreenLayersBot) {
    silkscreenBotLayers.appendChild(SExpression::createToken(layer));
  }
  silkscreenBot.ensureLineBreak();
  root.ensureLineBreak();

  SExpression& drills = root.appendList("drills");
  drills.appendChild("merge", mMergeDrillFiles);
  drills.ensureLineBreak();
  drills.appendChild("suffix_pth", mSuffixDrillsPth);
  drills.ensureLineBreak();
  drills.appendChild("suffix_npth", mSuffixDrillsNpth);
  drills.ensureLineBreak();
  drills.appendChild("suffix_merged", mSuffixDrills);
  drills.ensureLineBreak();
  root.ensureLineBreak();

  SExpression& solderPasteTop = root.appendList("solderpaste_top");
  solderPasteTop.appendChild("create", mEnableSolderPasteTop);
  solderPasteTop.appendChild("suffix", mSuffixSolderPasteTop);
  root.ensureLineBreak();

  SExpression& solderPasteBot = root.appendList("solderpaste_bot");
  solderPasteBot.appendChild("create", mEnableSolderPasteBot);
  solderPasteBot.appendChild("suffix", mSuffixSolderPasteBot);
  root.ensureLineBreak();
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

}  // namespace librepcb
