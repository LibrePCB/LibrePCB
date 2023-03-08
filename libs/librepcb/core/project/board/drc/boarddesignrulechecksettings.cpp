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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const BoardDesignRuleCheckSettings::AllowedSlots& obj) {
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
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings() noexcept
  : mMinCopperWidth(200000),  // 200um
    mMinCopperCopperClearance(200000),  // 200um
    mMinCopperBoardClearance(300000),  // 300um
    mMinCopperNpthClearance(250000),  // 250um
    mMinDrillDrillClearance(350000),  // 350um
    mMinDrillBoardClearance(500000),  // 500um
    mMinPthAnnularRing(200000),  // 200um
    mMinNpthDrillDiameter(300000),  // 300um
    mMinPthDrillDiameter(300000),  // 300um
    mMinNpthSlotWidth(1000000),  // 1mm
    mMinPthSlotWidth(700000),  // 0.7mm
    mAllowedNpthSlots(AllowedSlots::SingleSegmentStraight),
    mAllowedPthSlots(AllowedSlots::SingleSegmentStraight) {
}

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings(
    const BoardDesignRuleCheckSettings& other) noexcept
  : BoardDesignRuleCheckSettings()  // init and load defaults
{
  *this = other;
}

BoardDesignRuleCheckSettings::BoardDesignRuleCheckSettings(
    const SExpression& node)
  : mMinCopperWidth(
        deserialize<UnsignedLength>(node.getChild("min_copper_width/@0"))),
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
    mAllowedNpthSlots(
        deserialize<AllowedSlots>(node.getChild("allowed_npth_slots/@0"))),
    mAllowedPthSlots(
        deserialize<AllowedSlots>(node.getChild("allowed_pth_slots/@0"))) {
}

BoardDesignRuleCheckSettings::~BoardDesignRuleCheckSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRuleCheckSettings::serialize(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("min_copper_width", mMinCopperWidth);
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
  root.appendChild("allowed_npth_slots", mAllowedNpthSlots);
  root.ensureLineBreak();
  root.appendChild("allowed_pth_slots", mAllowedPthSlots);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoardDesignRuleCheckSettings& BoardDesignRuleCheckSettings::operator=(
    const BoardDesignRuleCheckSettings& rhs) noexcept {
  mMinCopperWidth = rhs.mMinCopperWidth;
  mMinCopperCopperClearance = rhs.mMinCopperCopperClearance;
  mMinCopperBoardClearance = rhs.mMinCopperBoardClearance;
  mMinCopperNpthClearance = rhs.mMinCopperNpthClearance;
  mMinDrillDrillClearance = rhs.mMinDrillDrillClearance;
  mMinDrillBoardClearance = rhs.mMinDrillBoardClearance;
  mMinPthAnnularRing = rhs.mMinPthAnnularRing;
  mMinNpthDrillDiameter = rhs.mMinNpthDrillDiameter;
  mMinPthDrillDiameter = rhs.mMinPthDrillDiameter;
  mMinNpthSlotWidth = rhs.mMinNpthSlotWidth;
  mMinPthSlotWidth = rhs.mMinPthSlotWidth;
  mAllowedNpthSlots = rhs.mAllowedNpthSlots;
  mAllowedPthSlots = rhs.mAllowedPthSlots;
  return *this;
}

bool BoardDesignRuleCheckSettings::operator==(
    const BoardDesignRuleCheckSettings& rhs) const noexcept {
  if (mMinCopperWidth != rhs.mMinCopperWidth) return false;
  if (mMinCopperCopperClearance != rhs.mMinCopperCopperClearance) return false;
  if (mMinCopperBoardClearance != rhs.mMinCopperBoardClearance) return false;
  if (mMinCopperNpthClearance != rhs.mMinCopperNpthClearance) return false;
  if (mMinDrillDrillClearance != rhs.mMinDrillDrillClearance) return false;
  if (mMinDrillBoardClearance != rhs.mMinDrillBoardClearance) return false;
  if (mMinPthAnnularRing != rhs.mMinPthAnnularRing) return false;
  if (mMinNpthDrillDiameter != rhs.mMinNpthDrillDiameter) return false;
  if (mMinPthDrillDiameter != rhs.mMinPthDrillDiameter) return false;
  if (mMinNpthSlotWidth != rhs.mMinNpthSlotWidth) return false;
  if (mMinPthSlotWidth != rhs.mMinPthSlotWidth) return false;
  if (mAllowedNpthSlots != rhs.mAllowedNpthSlots) return false;
  if (mAllowedPthSlots != rhs.mAllowedPthSlots) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
