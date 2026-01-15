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
#include "boarddesignrulechecksettings.h"

#include "../../../serialization/sexpression.h"
#include "../../../types/pcbcolor.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(
    const BoardDesignRuleCheckSettings::AllowedSlots& obj) {
  switch (obj) {
    case BoardDesignRuleCheckSettings::AllowedSlots::None:
      return SExpression::createToken("none");
    case BoardDesignRuleCheckSettings::AllowedSlots::SingleSegmentStraight:
      return SExpression::createToken("single_segment_straight");
    case BoardDesignRuleCheckSettings::AllowedSlots::MultiSegmentStraight:
      return SExpression::createToken("multi_segment_straight");
    case BoardDesignRuleCheckSettings::AllowedSlots::Any:
      return SExpression::createToken("any");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline BoardDesignRuleCheckSettings::AllowedSlots deserialize(
    const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("none")) {
    return BoardDesignRuleCheckSettings::AllowedSlots::None;
  } else if (str == QLatin1String("single_segment_straight")) {
    return BoardDesignRuleCheckSettings::AllowedSlots::SingleSegmentStraight;
  } else if (str == QLatin1String("multi_segment_straight")) {
    return BoardDesignRuleCheckSettings::AllowedSlots::MultiSegmentStraight;
  } else if (str == QLatin1String("any")) {
    return BoardDesignRuleCheckSettings::AllowedSlots::Any;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown allowed slots value: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Class BoardDesignRuleCheckSettings::Source
 ******************************************************************************/

BoardDesignRuleCheckSettings::Source BoardDesignRuleCheckSettings::Source::load(
    const SExpression& node) {
  return Source{
      deserialize<Uuid>(node.getChild("organization/@0")),
      deserialize<ElementName>(node.getChild("organization/@1")),
      deserialize<Version>(node.getChild("organization/@2")),
      deserialize<Uuid>(node.getChild("design_rules/@0")),
      deserialize<ElementName>(node.getChild("design_rules/@1")),
  };
}

void BoardDesignRuleCheckSettings::Source::serialize(SExpression& root) const {
  root.ensureLineBreak();
  SExpression& organization = root.appendList("organization");
  organization.appendChild(organizationUuid);
  organization.appendChild(organizationName);
  organization.appendChild(organizationVersion);
  root.ensureLineBreak();
  SExpression& rules = root.appendList("design_rules");
  rules.appendChild(pcbDesignRulesUuid);
  rules.appendChild(pcbDesignRulesName);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings() noexcept
  : mSources(),
    mMinBoardSize{UnsignedLength(0), UnsignedLength(0)},  // No minimum
    mMaxBoardSizeDoubleSided{UnsignedLength(0), UnsignedLength(0)},  // No limit
    mMaxBoardSizeMultiLayer{UnsignedLength(0), UnsignedLength(0)},  // No limit
    mPcbThickness(),  // No restrictions
    mMaxLayerCount(0),  // No restrictions
    mSolderResist(),  // No restrictions
    mSilkscreen(),  // No restrictions
    mMinCopperCopperClearance(200000),  // 200um
    mMinCopperBoardClearance(300000),  // 300um
    mMinCopperNpthClearance(250000),  // 250um
    mMinDrillDrillClearance(350000),  // 350um
    mMinDrillBoardClearance(500000),  // 500um
    mMinSilkscreenStopmaskClearance(127000),  // 127um
    mMinCopperWidth(200000),  // 200um
    mMinPthAnnularRing(200000),  // 200um
    mMinNpthDrillDiameter(500000),  // 0.5mm
    mMinPthDrillDiameter(300000),  // 0.3mm
    mMinNpthSlotWidth(1000000),  // 1mm
    mMinPthSlotWidth(700000),  // 0.7mm
    mMaxTentedViaDrillDiameter(3000000),  // 0.3mm
    mMinSilkscreenWidth(150000),  // 150um
    mMinSilkscreenTextHeight(800000),  // 0.8mm
    mMinOutlineToolDiameter(2000000),  // 2mm
    mBlindViasAllowed(false),  // Just to be on the safe side
    mBuriedViasAllowed(false),  // Just to be on the safe side
    mAllowedNpthSlots(AllowedSlots::SingleSegmentStraight),
    mAllowedPthSlots(AllowedSlots::SingleSegmentStraight),
    mOptions() {
}

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings(
    const BoardDesignRuleCheckSettings& other) noexcept
  : BoardDesignRuleCheckSettings()  // init and load defaults
{
  *this = other;
}

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings(
    const SExpression& node)
  : mSources(),  // Initialized below.
    mMinBoardSize{
        deserialize<UnsignedLength>(node.getChild("min_pcb_size/@0")),
        deserialize<UnsignedLength>(node.getChild("min_pcb_size/@1"))},
    mMaxBoardSizeDoubleSided{deserialize<UnsignedLength>(
                                 node.getChild("max_pcb_size/double_sided/@0")),
                             deserialize<UnsignedLength>(node.getChild(
                                 "max_pcb_size/double_sided/@1"))},
    mMaxBoardSizeMultiLayer{deserialize<UnsignedLength>(
                                node.getChild("max_pcb_size/multilayer/@0")),
                            deserialize<UnsignedLength>(
                                node.getChild("max_pcb_size/multilayer/@1"))},
    mPcbThickness(),  // Initialized below.
    mMaxLayerCount(deserialize<uint>(node.getChild("max_layers/@0"))),
    mSolderResist(),  // Initialized below.
    mSilkscreen(),  // Initialized below.
    mMinCopperCopperClearance(deserialize<UnsignedLength>(
        node.getChild("min_copper_copper_clearance/@0"))),
    mMinCopperBoardClearance(deserialize<UnsignedLength>(
        node.getChild("min_copper_board_clearance/@0"))),
    mMinCopperNpthClearance(deserialize<UnsignedLength>(
        node.getChild("min_copper_npth_clearance/@0"))),
    mMinDrillDrillClearance(deserialize<UnsignedLength>(
        node.getChild("min_drill_drill_clearance/@0"))),
    mMinDrillBoardClearance(deserialize<UnsignedLength>(
        node.getChild("min_drill_board_clearance/@0"))),
    mMinSilkscreenStopmaskClearance(deserialize<UnsignedLength>(
        node.getChild("min_silkscreen_stopmask_clearance/@0"))),
    mMinCopperWidth(
        deserialize<UnsignedLength>(node.getChild("min_copper_width/@0"))),
    mMinPthAnnularRing(
        deserialize<UnsignedLength>(node.getChild("min_annular_ring/@0"))),
    mMinNpthDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("min_npth_drill_diameter/@0"))),
    mMinPthDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("min_pth_drill_diameter/@0"))),
    mMinNpthSlotWidth(
        deserialize<UnsignedLength>(node.getChild("min_npth_slot_width/@0"))),
    mMinPthSlotWidth(
        deserialize<UnsignedLength>(node.getChild("min_pth_slot_width/@0"))),
    mMaxTentedViaDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("max_tented_via_drill_diameter/@0"))),
    mMinSilkscreenWidth(
        deserialize<UnsignedLength>(node.getChild("min_silkscreen_width/@0"))),
    mMinSilkscreenTextHeight(deserialize<UnsignedLength>(
        node.getChild("min_silkscreen_text_height/@0"))),
    mMinOutlineToolDiameter(deserialize<UnsignedLength>(
        node.getChild("min_outline_tool_diameter/@0"))),
    mBlindViasAllowed(
        deserialize<bool>(node.getChild("blind_vias_allowed/@0"))),
    mBuriedViasAllowed(
        deserialize<bool>(node.getChild("buried_vias_allowed/@0"))),
    mAllowedNpthSlots(
        deserialize<AllowedSlots>(node.getChild("allowed_npth_slots/@0"))),
    mAllowedPthSlots(
        deserialize<AllowedSlots>(node.getChild("allowed_pth_slots/@0"))),
    mOptions() {
  for (const SExpression* child : node.getChildren("source")) {
    mSources.insert(Source::load(*child));
  }
  for (const SExpression* child :
       node.getChild("pcb_thickness").getChildren(SExpression::Type::Token)) {
    mPcbThickness.insert(deserialize<PositiveLength>(*child));
  }
  for (const SExpression* child :
       node.getChild("solder_resist").getChildren(SExpression::Type::Token)) {
    mSolderResist.insert(deserialize<const PcbColor*>(*child));
  }
  for (const SExpression* child :
       node.getChild("silkscreen").getChildren(SExpression::Type::Token)) {
    mSilkscreen.insert(deserialize<const PcbColor*>(*child));
  }
  foreach (const SExpression* child, node.getChildren("option")) {
    mOptions[child->getChild("@0").getValue()].append(*child);
  }
}

BoardDesignRuleCheckSettings::~BoardDesignRuleCheckSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRuleCheckSettings::serialize(SExpression& root) const {
  auto cmpColor = [](const PcbColor* a, const PcbColor* b) {
    if ((!a) || (!b)) {
      return !b;
    } else {
      return a->getId() < b->getId();
    }
  };

  for (const Source& src : mSources) {
    root.ensureLineBreak();
    src.serialize(root.appendList("source"));
  }
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("min_pcb_size");
    child.appendChild(mMinBoardSize.first);
    child.appendChild(mMinBoardSize.second);
  }
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("max_pcb_size");
    SExpression& doubleSided = child.appendList("double_sided");
    doubleSided.appendChild(mMaxBoardSizeDoubleSided.first);
    doubleSided.appendChild(mMaxBoardSizeDoubleSided.second);
    SExpression& multilayer = child.appendList("multilayer");
    multilayer.appendChild(mMaxBoardSizeMultiLayer.first);
    multilayer.appendChild(mMaxBoardSizeMultiLayer.second);
  }
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("pcb_thickness");
    for (const PositiveLength& value : Toolbox::sortedQSet(mPcbThickness)) {
      child.appendChild(value);
    }
  }
  root.ensureLineBreak();
  root.appendChild("max_layers", mMaxLayerCount);
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("solder_resist");
    for (const PcbColor* value : Toolbox::sortedQSet(mSolderResist, cmpColor)) {
      child.appendChild(value);
    }
  }
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("silkscreen");
    for (const PcbColor* value : Toolbox::sortedQSet(mSilkscreen, cmpColor)) {
      child.appendChild(value);
    }
  }
  root.ensureLineBreak();
  root.appendChild("min_copper_copper_clearance", mMinCopperCopperClearance);
  root.ensureLineBreak();
  root.appendChild("min_copper_board_clearance", mMinCopperBoardClearance);
  root.ensureLineBreak();
  root.appendChild("min_copper_npth_clearance", mMinCopperNpthClearance);
  root.ensureLineBreak();
  root.appendChild("min_drill_drill_clearance", mMinDrillDrillClearance);
  root.ensureLineBreak();
  root.appendChild("min_drill_board_clearance", mMinDrillBoardClearance);
  root.ensureLineBreak();
  root.appendChild("min_silkscreen_stopmask_clearance",
                   mMinSilkscreenStopmaskClearance);
  root.ensureLineBreak();
  root.appendChild("min_copper_width", mMinCopperWidth);
  root.ensureLineBreak();
  root.appendChild("min_annular_ring", mMinPthAnnularRing);
  root.ensureLineBreak();
  root.appendChild("min_npth_drill_diameter", mMinNpthDrillDiameter);
  root.ensureLineBreak();
  root.appendChild("min_pth_drill_diameter", mMinPthDrillDiameter);
  root.ensureLineBreak();
  root.appendChild("min_npth_slot_width", mMinNpthSlotWidth);
  root.ensureLineBreak();
  root.appendChild("min_pth_slot_width", mMinPthSlotWidth);
  root.ensureLineBreak();
  root.appendChild("max_tented_via_drill_diameter", mMaxTentedViaDrillDiameter);
  root.ensureLineBreak();
  root.appendChild("min_silkscreen_width", mMinSilkscreenWidth);
  root.ensureLineBreak();
  root.appendChild("min_silkscreen_text_height", mMinSilkscreenTextHeight);
  root.ensureLineBreak();
  root.appendChild("min_outline_tool_diameter", mMinOutlineToolDiameter);
  root.ensureLineBreak();
  root.appendChild("blind_vias_allowed", mBlindViasAllowed);
  root.ensureLineBreak();
  root.appendChild("buried_vias_allowed", mBuriedViasAllowed);
  root.ensureLineBreak();
  root.appendChild("allowed_npth_slots", mAllowedNpthSlots);
  root.ensureLineBreak();
  root.appendChild("allowed_pth_slots", mAllowedPthSlots);
  root.ensureLineBreak();
  foreach (const auto& list, mOptions) {
    foreach (const auto& node, list) {
      root.ensureLineBreak();
      root.appendChild(node);
    }
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoardDesignRuleCheckSettings& BoardDesignRuleCheckSettings::operator=(
    const BoardDesignRuleCheckSettings& rhs) noexcept {
  mSources = rhs.mSources;
  mMinBoardSize = rhs.mMinBoardSize;
  mMaxBoardSizeDoubleSided = rhs.mMaxBoardSizeDoubleSided;
  mMaxBoardSizeMultiLayer = rhs.mMaxBoardSizeMultiLayer;
  mPcbThickness = rhs.mPcbThickness;
  mMaxLayerCount = rhs.mMaxLayerCount;
  mSolderResist = rhs.mSolderResist;
  mSilkscreen = rhs.mSilkscreen;
  mMinCopperCopperClearance = rhs.mMinCopperCopperClearance;
  mMinCopperBoardClearance = rhs.mMinCopperBoardClearance;
  mMinCopperNpthClearance = rhs.mMinCopperNpthClearance;
  mMinDrillDrillClearance = rhs.mMinDrillDrillClearance;
  mMinDrillBoardClearance = rhs.mMinDrillBoardClearance;
  mMinSilkscreenStopmaskClearance = rhs.mMinSilkscreenStopmaskClearance;
  mMinCopperWidth = rhs.mMinCopperWidth;
  mMinPthAnnularRing = rhs.mMinPthAnnularRing;
  mMinNpthDrillDiameter = rhs.mMinNpthDrillDiameter;
  mMinPthDrillDiameter = rhs.mMinPthDrillDiameter;
  mMinNpthSlotWidth = rhs.mMinNpthSlotWidth;
  mMinPthSlotWidth = rhs.mMinPthSlotWidth;
  mMaxTentedViaDrillDiameter = rhs.mMaxTentedViaDrillDiameter;
  mMinSilkscreenWidth = rhs.mMinSilkscreenWidth;
  mMinSilkscreenTextHeight = rhs.mMinSilkscreenTextHeight;
  mMinOutlineToolDiameter = rhs.mMinOutlineToolDiameter;
  mBlindViasAllowed = rhs.mBlindViasAllowed;
  mBuriedViasAllowed = rhs.mBuriedViasAllowed;
  mAllowedNpthSlots = rhs.mAllowedNpthSlots;
  mAllowedPthSlots = rhs.mAllowedPthSlots;
  mOptions = rhs.mOptions;
  return *this;
}

bool BoardDesignRuleCheckSettings::operator==(
    const BoardDesignRuleCheckSettings& rhs) const noexcept {
  if (mSources != rhs.mSources) return false;
  if (mMinBoardSize != rhs.mMinBoardSize) return false;
  if (mMaxBoardSizeDoubleSided != rhs.mMaxBoardSizeDoubleSided) return false;
  if (mMaxBoardSizeMultiLayer != rhs.mMaxBoardSizeMultiLayer) return false;
  if (mPcbThickness != rhs.mPcbThickness) return false;
  if (mMaxLayerCount != rhs.mMaxLayerCount) return false;
  if (mSolderResist != rhs.mSolderResist) return false;
  if (mSilkscreen != rhs.mSilkscreen) return false;
  if (mMinCopperCopperClearance != rhs.mMinCopperCopperClearance) return false;
  if (mMinCopperBoardClearance != rhs.mMinCopperBoardClearance) return false;
  if (mMinCopperNpthClearance != rhs.mMinCopperNpthClearance) return false;
  if (mMinDrillDrillClearance != rhs.mMinDrillDrillClearance) return false;
  if (mMinDrillBoardClearance != rhs.mMinDrillBoardClearance) return false;
  if (mMinSilkscreenStopmaskClearance != rhs.mMinSilkscreenStopmaskClearance)
    return false;
  if (mMinCopperWidth != rhs.mMinCopperWidth) return false;
  if (mMinPthAnnularRing != rhs.mMinPthAnnularRing) return false;
  if (mMinNpthDrillDiameter != rhs.mMinNpthDrillDiameter) return false;
  if (mMinPthDrillDiameter != rhs.mMinPthDrillDiameter) return false;
  if (mMinNpthSlotWidth != rhs.mMinNpthSlotWidth) return false;
  if (mMinPthSlotWidth != rhs.mMinPthSlotWidth) return false;
  if (mMaxTentedViaDrillDiameter != rhs.mMaxTentedViaDrillDiameter)
    return false;
  if (mMinSilkscreenWidth != rhs.mMinSilkscreenWidth) return false;
  if (mMinSilkscreenTextHeight != rhs.mMinSilkscreenTextHeight) return false;
  if (mMinOutlineToolDiameter != rhs.mMinOutlineToolDiameter) return false;
  if (mBlindViasAllowed != rhs.mBlindViasAllowed) return false;
  if (mBuriedViasAllowed != rhs.mBuriedViasAllowed) return false;
  if (mAllowedNpthSlots != rhs.mAllowedNpthSlots) return false;
  if (mAllowedPthSlots != rhs.mAllowedPthSlots) return false;
  if (mOptions != rhs.mOptions) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
