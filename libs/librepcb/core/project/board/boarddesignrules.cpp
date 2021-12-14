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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRules::BoardDesignRules() noexcept
  :  // general attributes
    mName(tr("LibrePCB Default Design Rules")),
    mDescription(),
    // stop mask
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

BoardDesignRules::BoardDesignRules(const SExpression& node,
                                   const Version& fileFormat)
  : BoardDesignRules()  // this loads all default values!
{
  // general attributes
  mName = deserialize<ElementName>(node.getChild("name/@0"), fileFormat);
  mDescription = node.getChild("description/@0").getValue();
  // stop mask
  if (const SExpression* e = node.tryGetChild("stopmask_clearance_ratio")) {
    mStopMaskClearanceRatio =
        deserialize<UnsignedRatio>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("stopmask_clearance_min")) {
    mStopMaskClearanceMin =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("stopmask_clearance_max")) {
    mStopMaskClearanceMax =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e =
          node.tryGetChild("stopmask_max_via_drill_diameter")) {
    mStopMaskMaxViaDrillDiameter =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  // cream mask
  if (const SExpression* e = node.tryGetChild("creammask_clearance_ratio")) {
    mCreamMaskClearanceRatio =
        deserialize<UnsignedRatio>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("creammask_clearance_min")) {
    mCreamMaskClearanceMin =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("creammask_clearance_max")) {
    mCreamMaskClearanceMax =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  // restring
  if (const SExpression* e = node.tryGetChild("restring_pad_ratio")) {
    mRestringPadRatio =
        deserialize<UnsignedRatio>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("restring_pad_min")) {
    mRestringPadMin =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("restring_pad_max")) {
    mRestringPadMax =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("restring_via_ratio")) {
    mRestringViaRatio =
        deserialize<UnsignedRatio>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("restring_via_min")) {
    mRestringViaMin =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }
  if (const SExpression* e = node.tryGetChild("restring_via_max")) {
    mRestringViaMax =
        deserialize<UnsignedLength>(e->getChild("@0"), fileFormat);
  }

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
  // general attributes
  root.appendChild("name", mName, true);
  root.appendChild("description", mDescription, true);
  // stop mask
  root.appendChild("stopmask_clearance_ratio", mStopMaskClearanceRatio, true);
  root.appendChild("stopmask_clearance_min", mStopMaskClearanceMin, true);
  root.appendChild("stopmask_clearance_max", mStopMaskClearanceMax, true);
  root.appendChild("stopmask_max_via_drill_diameter",
                   mStopMaskMaxViaDrillDiameter, true);
  // cream mask
  root.appendChild("creammask_clearance_ratio", mCreamMaskClearanceRatio, true);
  root.appendChild("creammask_clearance_min", mCreamMaskClearanceMin, true);
  root.appendChild("creammask_clearance_max", mCreamMaskClearanceMax, true);
  // restring
  root.appendChild("restring_pad_ratio", mRestringPadRatio, true);
  root.appendChild("restring_pad_min", mRestringPadMin, true);
  root.appendChild("restring_pad_max", mRestringPadMax, true);
  root.appendChild("restring_via_ratio", mRestringViaRatio, true);
  root.appendChild("restring_via_min", mRestringViaMin, true);
  root.appendChild("restring_via_max", mRestringViaMax, true);
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
  // general attributes
  mName = rhs.mName;
  mDescription = rhs.mDescription;
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
