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
    const SExpression& node)
  :
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mNames(node),
    mDescriptions(node),
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
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
