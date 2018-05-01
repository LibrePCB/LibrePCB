/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/graphics/graphicslayer.h>
#include "boardfabricationoutputsettings.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardFabricationOutputSettings::BoardFabricationOutputSettings() noexcept :
    mOutputBasePath("./output/{{VERSION}}/gerber/{{PROJECT}}"),
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
    mSilkscreenLayersTop({GraphicsLayer::sTopPlacement, GraphicsLayer::sTopNames}),
    mSilkscreenLayersBot({GraphicsLayer::sBotPlacement, GraphicsLayer::sBotNames}),
    mMergeDrillFiles(false),
    mEnableSolderPasteTop(false),
    mEnableSolderPasteBot(false)
{
}

BoardFabricationOutputSettings::BoardFabricationOutputSettings(const BoardFabricationOutputSettings& other) noexcept :
    BoardFabricationOutputSettings() // init and load defaults
{
    *this = other;
}

BoardFabricationOutputSettings::BoardFabricationOutputSettings(const SExpression& node) :
    BoardFabricationOutputSettings() // init and load defaults
{
    mOutputBasePath        = node.getValueByPath<QString>("base_path"              , false);
    mSuffixOutlines        = node.getValueByPath<QString>("outlines/suffix"        , false);
    mSuffixCopperTop       = node.getValueByPath<QString>("copper_top/suffix"      , false);
    mSuffixCopperInner     = node.getValueByPath<QString>("copper_inner/suffix"    , false);
    mSuffixCopperBot       = node.getValueByPath<QString>("copper_bot/suffix"      , false);
    mSuffixSolderMaskTop   = node.getValueByPath<QString>("soldermask_top/suffix"  , false);
    mSuffixSolderMaskBot   = node.getValueByPath<QString>("soldermask_bot/suffix"  , false);
    mSuffixSilkscreenTop   = node.getValueByPath<QString>("silkscreen_top/suffix"  , false);
    mSuffixSilkscreenBot   = node.getValueByPath<QString>("silkscreen_bot/suffix"  , false);
    mSuffixSolderPasteTop  = node.getValueByPath<QString>("solderpaste_top/suffix" , false);
    mSuffixSolderPasteBot  = node.getValueByPath<QString>("solderpaste_bot/suffix" , false);
    mSuffixDrillsPth       = node.getValueByPath<QString>("drills/suffix_pth"      , false);
    mSuffixDrillsNpth      = node.getValueByPath<QString>("drills/suffix_npth"     , false);
    mSuffixDrills          = node.getValueByPath<QString>("drills/suffix_merged"   , false);
    mMergeDrillFiles       = node.getValueByPath<bool   >("drills/merge"           , false);
    mEnableSolderPasteTop  = node.getValueByPath<bool   >("solderpaste_top/create" , false);
    mEnableSolderPasteBot  = node.getValueByPath<bool   >("solderpaste_bot/create" , false);

    mSilkscreenLayersTop.clear();
    foreach (const SExpression& child, node.getChildByPath("silkscreen_top/layers").getChildren()) {
        mSilkscreenLayersTop.append(child.getValue<QString>(false));
    }

    mSilkscreenLayersBot.clear();
    foreach (const SExpression& child, node.getChildByPath("silkscreen_bot/layers").getChildren()) {
        mSilkscreenLayersBot.append(child.getValue<QString>(false));
    }
}

BoardFabricationOutputSettings::~BoardFabricationOutputSettings() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardFabricationOutputSettings::serialize(SExpression& root) const
{
    root.appendStringChild("base_path"       , mOutputBasePath       , true);
    root.appendList("outlines"        , true).appendStringChild("suffix", mSuffixOutlines       , false);
    root.appendList("copper_top"      , true).appendStringChild("suffix", mSuffixCopperTop      , false);
    root.appendList("copper_inner"    , true).appendStringChild("suffix", mSuffixCopperInner    , false);
    root.appendList("copper_bot"      , true).appendStringChild("suffix", mSuffixCopperBot      , false);
    root.appendList("soldermask_top"  , true).appendStringChild("suffix", mSuffixSolderMaskTop  , false);
    root.appendList("soldermask_bot"  , true).appendStringChild("suffix", mSuffixSolderMaskBot  , false);

    SExpression& silkscreenTop = root.appendList("silkscreen_top", true);
    silkscreenTop.appendStringChild("suffix", mSuffixSilkscreenTop, false);
    SExpression& silkscreenTopLayers = silkscreenTop.appendList("layers", true);
    foreach (const QString& layer, mSilkscreenLayersTop) silkscreenTopLayers.appendToken(layer);

    SExpression& silkscreenBot = root.appendList("silkscreen_bot", true);
    silkscreenBot.appendStringChild("suffix", mSuffixSilkscreenBot, false);
    SExpression& silkscreenBotLayers = silkscreenBot.appendList("layers", true);
    foreach (const QString& layer, mSilkscreenLayersBot) silkscreenBotLayers.appendToken(layer);

    SExpression& drills = root.appendList("drills", true);
    drills.appendTokenChild("merge", mMergeDrillFiles, false);
    drills.appendStringChild("suffix_pth"   , mSuffixDrillsPth , true);
    drills.appendStringChild("suffix_npth"  , mSuffixDrillsNpth, true);
    drills.appendStringChild("suffix_merged", mSuffixDrills    , true);

    SExpression& solderPasteTop = root.appendList("solderpaste_top", true);
    solderPasteTop.appendTokenChild("create", mEnableSolderPasteTop, false);
    solderPasteTop.appendStringChild("suffix", mSuffixSolderPasteTop, false);

    SExpression& solderPasteBot = root.appendList("solderpaste_bot", true);
    solderPasteBot.appendTokenChild("create", mEnableSolderPasteBot, false);
    solderPasteBot.appendStringChild("suffix", mSuffixSolderPasteBot, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

BoardFabricationOutputSettings& BoardFabricationOutputSettings::operator=(
    const BoardFabricationOutputSettings& rhs) noexcept
{
    mOutputBasePath        = rhs.mOutputBasePath       ;
    mSuffixDrills          = rhs.mSuffixDrills         ;
    mSuffixDrillsNpth      = rhs.mSuffixDrillsNpth     ;
    mSuffixDrillsPth       = rhs.mSuffixDrillsPth      ;
    mSuffixOutlines        = rhs.mSuffixOutlines       ;
    mSuffixCopperTop       = rhs.mSuffixCopperTop      ;
    mSuffixCopperInner     = rhs.mSuffixCopperInner    ;
    mSuffixCopperBot       = rhs.mSuffixCopperBot      ;
    mSuffixSolderMaskTop   = rhs.mSuffixSolderMaskTop  ;
    mSuffixSolderMaskBot   = rhs.mSuffixSolderMaskBot  ;
    mSuffixSilkscreenTop   = rhs.mSuffixSilkscreenTop  ;
    mSuffixSilkscreenBot   = rhs.mSuffixSilkscreenBot  ;
    mSuffixSolderPasteTop  = rhs.mSuffixSolderPasteTop ;
    mSuffixSolderPasteBot  = rhs.mSuffixSolderPasteBot ;
    mSilkscreenLayersTop   = rhs.mSilkscreenLayersTop  ;
    mSilkscreenLayersBot   = rhs.mSilkscreenLayersBot  ;
    mMergeDrillFiles       = rhs.mMergeDrillFiles      ;
    mEnableSolderPasteTop  = rhs.mEnableSolderPasteTop ;
    mEnableSolderPasteBot  = rhs.mEnableSolderPasteBot ;
    return *this;
}

bool BoardFabricationOutputSettings::operator==(const BoardFabricationOutputSettings& rhs) const noexcept
{
    if (mOutputBasePath        != rhs.mOutputBasePath       ) return false;
    if (mSuffixDrills          != rhs.mSuffixDrills         ) return false;
    if (mSuffixDrillsNpth      != rhs.mSuffixDrillsNpth     ) return false;
    if (mSuffixDrillsPth       != rhs.mSuffixDrillsPth      ) return false;
    if (mSuffixOutlines        != rhs.mSuffixOutlines       ) return false;
    if (mSuffixCopperTop       != rhs.mSuffixCopperTop      ) return false;
    if (mSuffixCopperInner     != rhs.mSuffixCopperInner    ) return false;
    if (mSuffixCopperBot       != rhs.mSuffixCopperBot      ) return false;
    if (mSuffixSolderMaskTop   != rhs.mSuffixSolderMaskTop  ) return false;
    if (mSuffixSolderMaskBot   != rhs.mSuffixSolderMaskBot  ) return false;
    if (mSuffixSilkscreenTop   != rhs.mSuffixSilkscreenTop  ) return false;
    if (mSuffixSilkscreenBot   != rhs.mSuffixSilkscreenBot  ) return false;
    if (mSuffixSolderPasteTop  != rhs.mSuffixSolderPasteTop ) return false;
    if (mSuffixSolderPasteBot  != rhs.mSuffixSolderPasteBot ) return false;
    if (mSilkscreenLayersTop   != rhs.mSilkscreenLayersTop  ) return false;
    if (mSilkscreenLayersBot   != rhs.mSilkscreenLayersBot  ) return false;
    if (mMergeDrillFiles       != rhs.mMergeDrillFiles      ) return false;
    if (mEnableSolderPasteTop  != rhs.mEnableSolderPasteTop ) return false;
    if (mEnableSolderPasteBot  != rhs.mEnableSolderPasteBot ) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
