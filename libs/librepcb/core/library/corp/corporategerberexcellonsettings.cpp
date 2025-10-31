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
#include "corporategerberexcellonsettings.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CorporateGerberExcellonSettings::CorporateGerberExcellonSettings() noexcept
  : mDefault(true),
    mSuffixDrills("_DRILLS.drl"),
    mSuffixDrillsNpth("_DRILLS-NPTH.drl"),
    mSuffixDrillsPth("_DRILLS-PTH.drl"),
    mSuffixDrillsBlindBuried(
        "_DRILLS-PLATED-{{START_LAYER}}-{{END_LAYER}}.drl"),
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
    mMergeDrillFiles(false),
    mUseG85SlotCommand(false) {
}

CorporateGerberExcellonSettings::CorporateGerberExcellonSettings(
    const CorporateGerberExcellonSettings& other) noexcept
  : CorporateGerberExcellonSettings()  // init and load defaults
{
  *this = other;
}

CorporateGerberExcellonSettings::CorporateGerberExcellonSettings(
    const SExpression& node)
  : mDefault(deserialize<bool>(node.getChild("default/@0"))),
    mSuffixDrills(node.getChild("drills/suffix_merged/@0").getValue()),
    mSuffixDrillsNpth(node.getChild("drills/suffix_npth/@0").getValue()),
    mSuffixDrillsPth(node.getChild("drills/suffix_pth/@0").getValue()),
    mSuffixDrillsBlindBuried(
        node.getChild("drills/suffix_buried/@0").getValue()),
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
    mMergeDrillFiles(deserialize<bool>(node.getChild("drills/merge/@0"))),
    mUseG85SlotCommand(
        deserialize<bool>(node.getChild("drills/g85_slots/@0"))) {
}

CorporateGerberExcellonSettings::~CorporateGerberExcellonSettings() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CorporateGerberExcellonSettings::serialize(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("default", mDefault);
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
  root.appendList("silkscreen_top").appendChild("suffix", mSuffixSilkscreenTop);
  root.ensureLineBreak();
  root.appendList("silkscreen_bot").appendChild("suffix", mSuffixSilkscreenBot);
  root.ensureLineBreak();
  root.appendList("solderpaste_top")
      .appendChild("suffix", mSuffixSolderPasteTop);
  root.ensureLineBreak();
  root.appendList("solderpaste_bot")
      .appendChild("suffix", mSuffixSolderPasteBot);
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
  drills.appendChild("suffix_buried", mSuffixDrillsBlindBuried);
  drills.ensureLineBreak();
  drills.appendChild("g85_slots", mUseG85SlotCommand);
  drills.ensureLineBreak();
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

CorporateGerberExcellonSettings& CorporateGerberExcellonSettings::operator=(
    const CorporateGerberExcellonSettings& rhs) noexcept {
  mDefault = rhs.mDefault;
  mSuffixDrills = rhs.mSuffixDrills;
  mSuffixDrillsNpth = rhs.mSuffixDrillsNpth;
  mSuffixDrillsPth = rhs.mSuffixDrillsPth;
  mSuffixDrillsBlindBuried = rhs.mSuffixDrillsBlindBuried;
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
  mMergeDrillFiles = rhs.mMergeDrillFiles;
  mUseG85SlotCommand = rhs.mUseG85SlotCommand;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
