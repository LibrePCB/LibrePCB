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
    mStopMaskMaxViaDrillDiameter(500000),  // 0.5mm
    mStopMaskClearanceRatio(Ratio::percent0()),  // 0%
    mStopMaskClearanceMin(100000),  // 0.1mm
    mStopMaskClearanceMax(100000),  // 0.1mm
    // solder paste
    mSolderPasteClearanceRatio(Ratio::percent100() / 10),  // 10%
    mSolderPasteClearanceMin(0),  // 0.0mm
    mSolderPasteClearanceMax(1000000),  // 1.0mm
    // pad annular ring
    mPadCmpSideAutoAnnularRing(false),
    mPadInnerAutoAnnularRing(true),
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
    mStopMaskMaxViaDrillDiameter(deserialize<UnsignedLength>(
        node.getChild("stopmask_max_via_drill_diameter/@0"))),
    mStopMaskClearanceRatio(deserialize<UnsignedRatio>(
        node.getChild("stopmask_clearance/ratio/@0"))),
    mStopMaskClearanceMin(deserialize<UnsignedLength>(
        node.getChild("stopmask_clearance/min/@0"))),
    mStopMaskClearanceMax(deserialize<UnsignedLength>(
        node.getChild("stopmask_clearance/max/@0"))),
    // solder paste
    mSolderPasteClearanceRatio(deserialize<UnsignedRatio>(
        node.getChild("solderpaste_clearance/ratio/@0"))),
    mSolderPasteClearanceMin(deserialize<UnsignedLength>(
        node.getChild("solderpaste_clearance/min/@0"))),
    mSolderPasteClearanceMax(deserialize<UnsignedLength>(
        node.getChild("solderpaste_clearance/max/@0"))),
    // pad annular ring
    mPadCmpSideAutoAnnularRing(
        parsePadAutoAnnular(node.getChild("pad_annular_ring/outer/@0"))),
    mPadInnerAutoAnnularRing(
        parsePadAutoAnnular(node.getChild("pad_annular_ring/inner/@0"))),
    mPadAnnularRingRatio(
        deserialize<UnsignedRatio>(node.getChild("pad_annular_ring/ratio/@0"))),
    mPadAnnularRingMin(
        deserialize<UnsignedLength>(node.getChild("pad_annular_ring/min/@0"))),
    mPadAnnularRingMax(
        deserialize<UnsignedLength>(node.getChild("pad_annular_ring/max/@0"))),
    // via annular ring
    mViaAnnularRingRatio(
        deserialize<UnsignedRatio>(node.getChild("via_annular_ring/ratio/@0"))),
    mViaAnnularRingMin(
        deserialize<UnsignedLength>(node.getChild("via_annular_ring/min/@0"))),
    mViaAnnularRingMax(
        deserialize<UnsignedLength>(node.getChild("via_annular_ring/max/@0"))) {
  // force validating properties, throw exception on error
  try {
    setStopMaskClearance(mStopMaskClearanceRatio, mStopMaskClearanceMin,
                         mStopMaskClearanceMax);
    setSolderPasteClearance(mSolderPasteClearanceRatio,
                            mSolderPasteClearanceMin, mSolderPasteClearanceMax);
    setPadAnnularRing(mPadAnnularRingRatio, mPadAnnularRingMin,
                      mPadAnnularRingMax);
    setViaAnnularRing(mViaAnnularRingRatio, mViaAnnularRingMin,
                      mViaAnnularRingMax);
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

void BoardDesignRules::setStopMaskClearance(const UnsignedRatio& ratio,
                                            const UnsignedLength& min,
                                            const UnsignedLength& max) {
  if (max >= min) {
    mStopMaskClearanceRatio = ratio;
    mStopMaskClearanceMin = min;
    mStopMaskClearanceMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Stop mask clearance: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setSolderPasteClearance(const UnsignedRatio& ratio,
                                               const UnsignedLength& min,
                                               const UnsignedLength& max) {
  if (max >= min) {
    mSolderPasteClearanceRatio = ratio;
    mSolderPasteClearanceMin = min;
    mSolderPasteClearanceMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Solder paste clearance: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setPadCmpSideAutoAnnularRing(bool enabled) noexcept {
  mPadCmpSideAutoAnnularRing = enabled;
}

void BoardDesignRules::setPadInnerAutoAnnularRing(bool enabled) noexcept {
  mPadInnerAutoAnnularRing = enabled;
}

void BoardDesignRules::setPadAnnularRing(const UnsignedRatio& ratio,
                                         const UnsignedLength& min,
                                         const UnsignedLength& max) {
  if (max >= min) {
    mPadAnnularRingRatio = ratio;
    mPadAnnularRingMin = min;
    mPadAnnularRingMax = max;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Pads annular ring: MAX must be >= MIN"));
  }
}

void BoardDesignRules::setViaAnnularRing(const UnsignedRatio& ratio,
                                         const UnsignedLength& min,
                                         const UnsignedLength& max) {
  if (max >= min) {
    mViaAnnularRingRatio = ratio;
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
  {
    root.ensureLineBreak();
    root.appendChild("stopmask_max_via_drill_diameter",
                     mStopMaskMaxViaDrillDiameter);
    root.ensureLineBreak();
    SExpression& node = root.appendList("stopmask_clearance");
    node.appendChild("ratio", mStopMaskClearanceRatio);
    node.appendChild("min", mStopMaskClearanceMin);
    node.appendChild("max", mStopMaskClearanceMax);
  }

  // solder paste
  {
    root.ensureLineBreak();
    SExpression& node = root.appendList("solderpaste_clearance");
    node.appendChild("ratio", mSolderPasteClearanceRatio);
    node.appendChild("min", mSolderPasteClearanceMin);
    node.appendChild("max", mSolderPasteClearanceMax);
  }

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
    node.appendChild("ratio", mPadAnnularRingRatio);
    node.appendChild("min", mPadAnnularRingMin);
    node.appendChild("max", mPadAnnularRingMax);
  }

  // via annular ring
  {
    root.ensureLineBreak();
    SExpression& node = root.appendList("via_annular_ring");
    node.appendChild("ratio", mViaAnnularRingRatio);
    node.appendChild("min", mViaAnnularRingMin);
    node.appendChild("max", mViaAnnularRingMax);
  }

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
  mStopMaskMaxViaDrillDiameter = rhs.mStopMaskMaxViaDrillDiameter;
  mStopMaskClearanceRatio = rhs.mStopMaskClearanceRatio;
  mStopMaskClearanceMin = rhs.mStopMaskClearanceMin;
  mStopMaskClearanceMax = rhs.mStopMaskClearanceMax;
  // solder paste
  mSolderPasteClearanceRatio = rhs.mSolderPasteClearanceRatio;
  mSolderPasteClearanceMin = rhs.mSolderPasteClearanceMin;
  mSolderPasteClearanceMax = rhs.mSolderPasteClearanceMax;
  // pad annular ring
  mPadCmpSideAutoAnnularRing = rhs.mPadCmpSideAutoAnnularRing;
  mPadInnerAutoAnnularRing = rhs.mPadInnerAutoAnnularRing;
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
