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
#include "pcbmanufacturercapabilities.h"

#include "../../serialization/sexpression.h"

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
    const PcbManufacturerCapabilities::AllowedSlots& obj) {
  switch (obj) {
    case PcbManufacturerCapabilities::AllowedSlots::None:
      return SExpression::createToken("none");
    case PcbManufacturerCapabilities::AllowedSlots::SingleSegmentStraight:
      return SExpression::createToken("single_segment_straight");
    case PcbManufacturerCapabilities::AllowedSlots::MultiSegmentStraight:
      return SExpression::createToken("multi_segment_straight");
    case PcbManufacturerCapabilities::AllowedSlots::Any:
      return SExpression::createToken("any");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline PcbManufacturerCapabilities::AllowedSlots deserialize(
    const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("none")) {
    return PcbManufacturerCapabilities::AllowedSlots::None;
  } else if (str == QLatin1String("single_segment_straight")) {
    return PcbManufacturerCapabilities::AllowedSlots::SingleSegmentStraight;
  } else if (str == QLatin1String("multi_segment_straight")) {
    return PcbManufacturerCapabilities::AllowedSlots::MultiSegmentStraight;
  } else if (str == QLatin1String("any")) {
    return PcbManufacturerCapabilities::AllowedSlots::Any;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown allowed slots value: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PcbManufacturerCapabilities::PcbManufacturerCapabilities(
    const PcbManufacturerCapabilities& other) noexcept
  : mUuid(other.mUuid),
    mNames(other.mNames),
    mDescriptions(other.mDescriptions),
    mUrl(other.mUrl),
    mMinBoardSize(other.mMinBoardSize),
    mMaxBoardSizeDoubleSided(other.mMaxBoardSizeDoubleSided),
    mMaxBoardSizeMultiLayer(other.mMaxBoardSizeMultiLayer),
    mPcbThickness(other.mPcbThickness),
    mMaxInnerLayerCount(other.mMaxInnerLayerCount),
    mSolderResist(other.mSolderResist),
    mSilkscreen(other.mSilkscreen),
    mMinCopperCopperClearance(other.mMinCopperCopperClearance),
    mMinCopperBoardClearance(other.mMinCopperBoardClearance),
    mMinCopperNpthClearance(other.mMinCopperNpthClearance),
    mMinDrillDrillClearance(other.mMinDrillDrillClearance),
    mMinDrillBoardClearance(other.mMinDrillBoardClearance),
    mMinSilkscreenStopmaskClearance(other.mMinSilkscreenStopmaskClearance),
    mMinCopperWidth(other.mMinCopperWidth),
    mMinPthAnnularRing(other.mMinPthAnnularRing),
    mMinNpthDrillDiameter(other.mMinNpthDrillDiameter),
    mMinPthDrillDiameter(other.mMinPthDrillDiameter),
    mMinNpthSlotWidth(other.mMinNpthSlotWidth),
    mMinPthSlotWidth(other.mMinPthSlotWidth),
    mMinSilkscreenWidth(other.mMinSilkscreenWidth),
    mMinSilkscreenTextHeight(other.mMinSilkscreenTextHeight),
    mMinOutlineToolDiameter(other.mMinOutlineToolDiameter),
    mBlindViasAllowed(other.mBlindViasAllowed),
    mBuriedViasAllowed(other.mBuriedViasAllowed),
    mAllowedNpthSlots(other.mAllowedNpthSlots),
    mAllowedPthSlots(other.mAllowedPthSlots) {
}

PcbManufacturerCapabilities::PcbManufacturerCapabilities(
    const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mNames(node),
    mDescriptions(node),
    // Note: Don't use SExpression::getValueByPath<QUrl>() because it would
    // throw an exception if the URL is empty, which is actually legal in this
    // case.
    mUrl(node.getChild("url/@0").getValue(), QUrl::StrictMode),
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
    mMaxInnerLayerCount(
        deserialize<uint>(node.getChild("max_inner_layers/@0"))),
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
        deserialize<AllowedSlots>(node.getChild("allowed_pth_slots/@0"))) {
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
}

PcbManufacturerCapabilities::~PcbManufacturerCapabilities() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PcbManufacturerCapabilities::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  mNames.serialize(root);
  root.ensureLineBreak();
  mDescriptions.serialize(root);
  root.ensureLineBreak();
  root.appendChild("url", mUrl);
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
    for (const PositiveLength& value : mPcbThickness) {
      child.appendChild(value);
    }
  }
  root.ensureLineBreak();
  root.appendChild("max_inner_layers", mMaxInnerLayerCount);
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("solder_resist");
    for (const PcbColor* value : mSolderResist) {
      child.appendChild(value);
    }
  }
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("silkscreen");
    for (const PcbColor* value : mSilkscreen) {
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
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

PcbManufacturerCapabilities& PcbManufacturerCapabilities::operator=(
    const PcbManufacturerCapabilities& rhs) noexcept {
  mUuid = rhs.mUuid;
  mNames = rhs.mNames;
  mDescriptions = rhs.mDescriptions;
  mUrl = rhs.mUrl;
  mMinBoardSize = rhs.mMinBoardSize;
  mMaxBoardSizeDoubleSided = rhs.mMaxBoardSizeDoubleSided;
  mMaxBoardSizeMultiLayer = rhs.mMaxBoardSizeMultiLayer;
  mPcbThickness = rhs.mPcbThickness;
  mMaxInnerLayerCount = rhs.mMaxInnerLayerCount;
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
  mMinSilkscreenWidth = rhs.mMinSilkscreenWidth;
  mMinSilkscreenTextHeight = rhs.mMinSilkscreenTextHeight;
  mMinOutlineToolDiameter = rhs.mMinOutlineToolDiameter;
  mBlindViasAllowed = rhs.mBlindViasAllowed;
  mBuriedViasAllowed = rhs.mBuriedViasAllowed;
  mAllowedNpthSlots = rhs.mAllowedNpthSlots;
  mAllowedPthSlots = rhs.mAllowedPthSlots;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
