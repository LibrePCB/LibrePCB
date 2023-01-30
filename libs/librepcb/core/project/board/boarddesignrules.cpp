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
  :  // stop mask
    mStopMaskClearanceRatio(Ratio::percent0()),  // 0%
    mStopMaskClearanceMin(100000),  // 0.1mm
    mStopMaskClearanceMax(100000),  // 0.1mm
    mStopMaskMaxViaDrillDiameter(500000),  // 0.5mm
    // solder paste
    mSolderPasteClearanceRatio(Ratio::percent100() / 10),  // 10%
    mSolderPasteClearanceMin(0),  // 0.0mm
    mSolderPasteClearanceMax(1000000),  // 1.0mm
    // pad annular ring
    mPadAnnularRingRatio(Ratio::percent100() / 4),  // 25%
    mPadAnnularRingMin(250000),  // 0.25mm
    mPadAnnularRingMax(2000000),  // 2.0mm
    // via annular ring
    mViaAnnularRingRatio(Ratio::percent100() / 4),  // 25%
    mViaAnnularRingMin(200000),  // 0.2mm
    mViaAnnularRingMax(2000000)  // 2.0mm
{
}

BoardDesignRules::BoardDesignRules(const BoardDesignRules& other)
  : BoardDesignRules() {
  *this = other;
}

BoardDesignRules::BoardDesignRules(const SExpression& node)
  :  // stop mask
    mStopMaskClearanceRatio(deserialize<UnsignedRatio>(
        node.getChild("stopmask_clearance_ratio/@0"))),
    mStopMaskClearanceMin(deserialize<UnsignedLength>(
        node.getChild("stopmask_clearance_min/@0"))),
    mStopMaskClearanceMax(deserialize<UnsignedLength>(
        node.getChild("stopmask_clearance_max/@0"))),
    mStopMaskMaxViaDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("stopmask_max_via_drill_diameter/@0"))),
    // solder paste
    mSolderPasteClearanceRatio(deserialize<UnsignedRatio>(
        node.getChild("solderpaste_clearance_ratio/@0"))),
    mSolderPasteClearanceMin(deserialize<UnsignedLength>(
        node.getChild("solderpaste_clearance_min/@0"))),
    mSolderPasteClearanceMax(deserialize<UnsignedLength>(
        node.getChild("solderpaste_clearance_max/@0"))),
    // pad annular ring
    mPadAnnularRingRatio(
        deserialize<UnsignedRatio>(node.getChild("pad_annular_ring_ratio/@0"))),
    mPadAnnularRingMin(
        deserialize<UnsignedLength>(node.getChild("pad_annular_ring_min/@0"))),
    mPadAnnularRingMax(
        deserialize<UnsignedLength>(node.getChild("pad_annular_ring_max/@0"))),
    // via annular ring
    mViaAnnularRingRatio(
        deserialize<UnsignedRatio>(node.getChild("via_annular_ring_ratio/@0"))),
    mViaAnnularRingMin(
        deserialize<UnsignedLength>(node.getChild("via_annular_ring_min/@0"))),
    mViaAnnularRingMax(
        deserialize<UnsignedLength>(node.getChild("via_annular_ring_max/@0"))) {
  // force validating properties, throw exception on error
  try {
    setStopMaskClearanceBounds(mStopMaskClearanceMin, mStopMaskClearanceMax);
    setSolderPasteClearanceBounds(mSolderPasteClearanceMin,
                                  mSolderPasteClearanceMax);
    setPadAnnularRingBounds(mPadAnnularRingMin, mPadAnnularRingMax);
    setViaAnnularRingBounds(mViaAnnularRingMin, mViaAnnularRingMax);
  } catch (const Exception& e) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Invalid design rules: %1").arg(e.getMsg()));
  }
}

BoardDesignRules::~BoardDesignRules() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BoardDesignRules::setStopMaskClearanceBounds(const UnsignedLength& min,
                                                  const UnsignedLength& max) {
  if (max >= min) {
    mStopMaskClearanceMin = min;
    mStopMaskClearanceMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Stop mask clearance: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setSolderPasteClearanceBounds(
    const UnsignedLength& min, const UnsignedLength& max) {
  if (max >= min) {
    mSolderPasteClearanceMin = min;
    mSolderPasteClearanceMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Solder paste clearance: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setPadAnnularRingBounds(const UnsignedLength& min,
                                               const UnsignedLength& max) {
  if (max >= min) {
    mPadAnnularRingMin = min;
    mPadAnnularRingMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Pads annular ring: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setViaAnnularRingBounds(const UnsignedLength& min,
                                               const UnsignedLength& max) {
  if (max >= min) {
    mViaAnnularRingMin = min;
    mViaAnnularRingMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Vias annular ring: MAX must be >= MIN"));
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRules::restoreDefaults() noexcept {
  *this = BoardDesignRules();
}

void BoardDesignRules::serialize(SExpression& root) const {
  // stop mask
  root.ensureLineBreak();
  root.appendChild("stopmask_clearance_ratio", mStopMaskClearanceRatio);
  root.ensureLineBreak();
  root.appendChild("stopmask_clearance_min", mStopMaskClearanceMin);
  root.ensureLineBreak();
  root.appendChild("stopmask_clearance_max", mStopMaskClearanceMax);
  root.ensureLineBreak();
  root.appendChild("stopmask_max_via_drill_diameter",
                   mStopMaskMaxViaDrillDiameter);
  // solder paste
  root.ensureLineBreak();
  root.appendChild("solderpaste_clearance_ratio", mSolderPasteClearanceRatio);
  root.ensureLineBreak();
  root.appendChild("solderpaste_clearance_min", mSolderPasteClearanceMin);
  root.ensureLineBreak();
  root.appendChild("solderpaste_clearance_max", mSolderPasteClearanceMax);
  // pad annular ring
  root.ensureLineBreak();
  root.appendChild("pad_annular_ring_ratio", mPadAnnularRingRatio);
  root.ensureLineBreak();
  root.appendChild("pad_annular_ring_min", mPadAnnularRingMin);
  root.ensureLineBreak();
  root.appendChild("pad_annular_ring_max", mPadAnnularRingMax);
  // via annular ring
  root.ensureLineBreak();
  root.appendChild("via_annular_ring_ratio", mViaAnnularRingRatio);
  root.ensureLineBreak();
  root.appendChild("via_annular_ring_min", mViaAnnularRingMin);
  root.ensureLineBreak();
  root.appendChild("via_annular_ring_max", mViaAnnularRingMax);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Helper Methods
 ******************************************************************************/

bool BoardDesignRules::doesViaRequireStopMask(const Length& drillDia) const
    noexcept {
  return (drillDia > *mStopMaskMaxViaDrillDiameter ? true : false);
}

UnsignedLength BoardDesignRules::calcStopMaskClearance(
    const Length& padSize) const noexcept {
  return UnsignedLength(
      qBound(*mStopMaskClearanceMin,
             padSize.scaled(mStopMaskClearanceRatio->toNormalized()),
             *mStopMaskClearanceMax));
}

UnsignedLength BoardDesignRules::calcSolderPasteClearance(
    const Length& padSize) const noexcept {
  return UnsignedLength(
      qBound(*mSolderPasteClearanceMin,
             padSize.scaled(mSolderPasteClearanceRatio->toNormalized()),
             *mSolderPasteClearanceMax));
}

UnsignedLength BoardDesignRules::calcPadAnnularRing(
    const Length& drillDia) const noexcept {
  return UnsignedLength(
      qBound(*mPadAnnularRingMin,
             drillDia.scaled(mPadAnnularRingRatio->toNormalized()),
             *mPadAnnularRingMax));
}

UnsignedLength BoardDesignRules::calcViaAnnularRing(
    const Length& drillDia) const noexcept {
  return UnsignedLength(
      qBound(*mViaAnnularRingMin,
             drillDia.scaled(mViaAnnularRingRatio->toNormalized()),
             *mViaAnnularRingMax));
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoardDesignRules& BoardDesignRules::operator=(
    const BoardDesignRules& rhs) noexcept {
  // stop mask
  mStopMaskClearanceRatio = rhs.mStopMaskClearanceRatio;
  mStopMaskClearanceMin = rhs.mStopMaskClearanceMin;
  mStopMaskClearanceMax = rhs.mStopMaskClearanceMax;
  mStopMaskMaxViaDrillDiameter = rhs.mStopMaskMaxViaDrillDiameter;
  // solder paste
  mSolderPasteClearanceRatio = rhs.mSolderPasteClearanceRatio;
  mSolderPasteClearanceMin = rhs.mSolderPasteClearanceMin;
  mSolderPasteClearanceMax = rhs.mSolderPasteClearanceMax;
  // pad annular ring
  mPadAnnularRingRatio = rhs.mPadAnnularRingRatio;
  mPadAnnularRingMin = rhs.mPadAnnularRingMin;
  mPadAnnularRingMax = rhs.mPadAnnularRingMax;
  // via annular ring
  mViaAnnularRingRatio = rhs.mViaAnnularRingRatio;
  mViaAnnularRingMin = rhs.mViaAnnularRingMin;
  mViaAnnularRingMax = rhs.mViaAnnularRingMax;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
