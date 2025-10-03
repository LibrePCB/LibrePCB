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
#include "boarddesignrules.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRules::BoardDesignRules() noexcept
  :  // default values
    mDefaultTraceWidth(500000),  // 0.5mm
    mDefaultViaDrillDiameter(300000),  // 0.3mm
    // stop mask
    mStopMaskMaxViaDrillDiameter(500000),  // 0.5mm
    mStopMaskClearance(UnsignedRatio(Ratio::fromPercent(0)),  // 0%
                       UnsignedLength(100000),  // 0.1mm
                       UnsignedLength(100000)),  // 0.1mm
    // solder paste
    mSolderPasteClearance(UnsignedRatio(Ratio::fromPercent(10)),  // 10%
                          UnsignedLength(0),  // 0mm
                          UnsignedLength(1000000)),  // 1mm
    // pad annular ring
    mPadCmpSideAutoAnnularRing(false),
    mPadInnerAutoAnnularRing(true),
    mPadAnnularRing(UnsignedRatio(Ratio::fromPercent(25)),  // 25%
                    UnsignedLength(250000),  // 0.25mm
                    UnsignedLength(2000000)),  // 2mm
    // via annular ring
    mViaAnnularRing(UnsignedRatio(Ratio::fromPercent(25)),  // 25%
                    UnsignedLength(200000),  // 0.2mm
                    UnsignedLength(2000000))  // 2mm
{
}

BoardDesignRules::BoardDesignRules(const BoardDesignRules& other)
  : BoardDesignRules() {
  *this = other;
}

BoardDesignRules::BoardDesignRules(const SExpression& node)
  :  // default values
    mDefaultTraceWidth(
        deserialize<PositiveLength>(node.getChild("default_trace_width/@0"))),
    mDefaultViaDrillDiameter(deserialize<PositiveLength>(
        node.getChild("default_via_drill_diameter/@0"))),
    // stop mask
    mStopMaskMaxViaDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("stopmask_max_via_drill_diameter/@0"))),
    mStopMaskClearance(node.getChild("stopmask_clearance")),
    // solder paste
    mSolderPasteClearance(node.getChild("solderpaste_clearance")),
    // pad annular ring
    mPadCmpSideAutoAnnularRing(
        parsePadAutoAnnular(node.getChild("pad_annular_ring/outer/@0"))),
    mPadInnerAutoAnnularRing(
        parsePadAutoAnnular(node.getChild("pad_annular_ring/inner/@0"))),
    mPadAnnularRing(node.getChild("pad_annular_ring")),
    // via annular ring
    mViaAnnularRing(node.getChild("via_annular_ring")) {
}

BoardDesignRules::~BoardDesignRules() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRules::restoreDefaults() noexcept {
  *this = BoardDesignRules();
}

void BoardDesignRules::serialize(SExpression& root) const {
  // default values
  root.ensureLineBreak();
  root.appendChild("default_trace_width", mDefaultTraceWidth);
  root.ensureLineBreak();
  root.appendChild("default_via_drill_diameter", mDefaultViaDrillDiameter);

  // stop mask
  root.ensureLineBreak();
  root.appendChild("stopmask_max_via_drill_diameter",
                   mStopMaskMaxViaDrillDiameter);
  root.ensureLineBreak();
  mStopMaskClearance.serialize(root.appendList("stopmask_clearance"));

  // solder paste
  root.ensureLineBreak();
  mSolderPasteClearance.serialize(root.appendList("solderpaste_clearance"));

  // pad annular ring
  {
    root.ensureLineBreak();
    SExpression& node = root.appendList("pad_annular_ring");
    node.appendChild(
        "outer",
        SExpression::createToken(mPadCmpSideAutoAnnularRing ? "auto" : "full"));
    node.appendChild(
        "inner",
        SExpression::createToken(mPadInnerAutoAnnularRing ? "auto" : "full"));
    mPadAnnularRing.serialize(node);
  }

  // via annular ring
  root.ensureLineBreak();
  mViaAnnularRing.serialize(root.appendList("via_annular_ring"));

  root.ensureLineBreak();
}

/*******************************************************************************
 *  Helper Methods
 ******************************************************************************/

bool BoardDesignRules::doesViaRequireStopMaskOpening(
    const Length& drillDia) const noexcept {
  return drillDia > (*mStopMaskMaxViaDrillDiameter);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoardDesignRules& BoardDesignRules::operator=(
    const BoardDesignRules& rhs) noexcept {
  // default values
  mDefaultTraceWidth = rhs.mDefaultTraceWidth;
  mDefaultViaDrillDiameter = rhs.mDefaultViaDrillDiameter;
  // stop mask
  mStopMaskMaxViaDrillDiameter = rhs.mStopMaskMaxViaDrillDiameter;
  mStopMaskClearance = rhs.mStopMaskClearance;
  // solder paste
  mSolderPasteClearance = rhs.mSolderPasteClearance;
  // pad annular ring
  mPadCmpSideAutoAnnularRing = rhs.mPadCmpSideAutoAnnularRing;
  mPadInnerAutoAnnularRing = rhs.mPadInnerAutoAnnularRing;
  mPadAnnularRing = rhs.mPadAnnularRing;
  // via annular ring
  mViaAnnularRing = rhs.mViaAnnularRing;
  return *this;
}

bool BoardDesignRules::operator==(const BoardDesignRules& rhs) const noexcept {
  // default values
  if (mDefaultTraceWidth != rhs.mDefaultTraceWidth) return false;
  if (mDefaultViaDrillDiameter != rhs.mDefaultViaDrillDiameter) return false;
  // stop mask
  if (mStopMaskMaxViaDrillDiameter != rhs.mStopMaskMaxViaDrillDiameter)
    return false;
  if (mStopMaskClearance != rhs.mStopMaskClearance) return false;
  // solder paste
  if (mSolderPasteClearance != rhs.mSolderPasteClearance) return false;
  // pad annular ring
  if (mPadCmpSideAutoAnnularRing != rhs.mPadCmpSideAutoAnnularRing)
    return false;
  if (mPadInnerAutoAnnularRing != rhs.mPadInnerAutoAnnularRing) return false;
  if (mPadAnnularRing != rhs.mPadAnnularRing) return false;
  // via annular ring
  if (mViaAnnularRing != rhs.mViaAnnularRing) return false;
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardDesignRules::parsePadAutoAnnular(const SExpression& node) {
  const QString str = node.getValue();
  if (str == "auto") {
    return true;
  } else if (str == "full") {
    return false;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid pad annular shape: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
