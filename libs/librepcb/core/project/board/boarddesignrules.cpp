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
    // cream mask
    mCreamMaskClearanceRatio(Ratio::percent100() / 10),  // 10%
    mCreamMaskClearanceMin(0),  // 0.0mm
    mCreamMaskClearanceMax(1000000),  // 1.0mm
    // restring
    mRestringPadRatio(Ratio::percent100() / 4),  // 25%
    mRestringPadMin(250000),  // 0.25mm
    mRestringPadMax(2000000),  // 2.0mm
    mRestringViaRatio(Ratio::percent100() / 4),  // 25%
    mRestringViaMin(200000),  // 0.2mm
    mRestringViaMax(2000000)  // 2.0mm
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
    // cream mask
    mCreamMaskClearanceRatio(deserialize<UnsignedRatio>(
        node.getChild("creammask_clearance_ratio/@0"))),
    mCreamMaskClearanceMin(deserialize<UnsignedLength>(
        node.getChild("creammask_clearance_min/@0"))),
    mCreamMaskClearanceMax(deserialize<UnsignedLength>(
        node.getChild("creammask_clearance_max/@0"))),
    // restring
    mRestringPadRatio(
        deserialize<UnsignedRatio>(node.getChild("restring_pad_ratio/@0"))),
    mRestringPadMin(
        deserialize<UnsignedLength>(node.getChild("restring_pad_min/@0"))),
    mRestringPadMax(
        deserialize<UnsignedLength>(node.getChild("restring_pad_max/@0"))),
    mRestringViaRatio(
        deserialize<UnsignedRatio>(node.getChild("restring_via_ratio/@0"))),
    mRestringViaMin(
        deserialize<UnsignedLength>(node.getChild("restring_via_min/@0"))),
    mRestringViaMax(
        deserialize<UnsignedLength>(node.getChild("restring_via_max/@0"))) {
  // force validating properties, throw exception on error
  try {
    setStopMaskClearanceBounds(mStopMaskClearanceMin, mStopMaskClearanceMax);
    setCreamMaskClearanceBounds(mCreamMaskClearanceMin, mCreamMaskClearanceMax);
    setRestringPadBounds(mRestringPadMin, mRestringPadMax);
    setRestringViaBounds(mRestringViaMin, mRestringViaMax);
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

void BoardDesignRules::setCreamMaskClearanceBounds(const UnsignedLength& min,
                                                   const UnsignedLength& max) {
  if (max >= min) {
    mCreamMaskClearanceMin = min;
    mCreamMaskClearanceMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Cream mask clearance: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setRestringPadBounds(const UnsignedLength& min,
                                            const UnsignedLength& max) {
  if (max >= min) {
    mRestringPadMin = min;
    mRestringPadMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Restring pads: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setRestringViaBounds(const UnsignedLength& min,
                                            const UnsignedLength& max) {
  if (max >= min) {
    mRestringViaMin = min;
    mRestringViaMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Restring vias: MAX must be >= MIN"));
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
  // cream mask
  root.ensureLineBreak();
  root.appendChild("creammask_clearance_ratio", mCreamMaskClearanceRatio);
  root.ensureLineBreak();
  root.appendChild("creammask_clearance_min", mCreamMaskClearanceMin);
  root.ensureLineBreak();
  root.appendChild("creammask_clearance_max", mCreamMaskClearanceMax);
  // restring
  root.ensureLineBreak();
  root.appendChild("restring_pad_ratio", mRestringPadRatio);
  root.ensureLineBreak();
  root.appendChild("restring_pad_min", mRestringPadMin);
  root.ensureLineBreak();
  root.appendChild("restring_pad_max", mRestringPadMax);
  root.ensureLineBreak();
  root.appendChild("restring_via_ratio", mRestringViaRatio);
  root.ensureLineBreak();
  root.appendChild("restring_via_min", mRestringViaMin);
  root.ensureLineBreak();
  root.appendChild("restring_via_max", mRestringViaMax);
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

UnsignedLength BoardDesignRules::calcCreamMaskClearance(
    const Length& padSize) const noexcept {
  return UnsignedLength(
      qBound(*mCreamMaskClearanceMin,
             padSize.scaled(mCreamMaskClearanceRatio->toNormalized()),
             *mCreamMaskClearanceMax));
}

UnsignedLength BoardDesignRules::calcPadRestring(const Length& drillDia) const
    noexcept {
  return UnsignedLength(qBound(
      *mRestringPadMin, drillDia.scaled(mRestringPadRatio->toNormalized()),
      *mRestringPadMax));
}

UnsignedLength BoardDesignRules::calcViaRestring(const Length& drillDia) const
    noexcept {
  return UnsignedLength(qBound(
      *mRestringViaMin, drillDia.scaled(mRestringViaRatio->toNormalized()),
      *mRestringViaMax));
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
  // cream mask
  mCreamMaskClearanceRatio = rhs.mCreamMaskClearanceRatio;
  mCreamMaskClearanceMin = rhs.mCreamMaskClearanceMin;
  mCreamMaskClearanceMax = rhs.mCreamMaskClearanceMax;
  // restring
  mRestringPadRatio = rhs.mRestringPadRatio;
  mRestringPadMin = rhs.mRestringPadMin;
  mRestringPadMax = rhs.mRestringPadMax;
  mRestringViaRatio = rhs.mRestringViaRatio;
  mRestringViaMin = rhs.mRestringViaMin;
  mRestringViaMax = rhs.mRestringViaMax;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
