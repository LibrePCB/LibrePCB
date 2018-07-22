/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "boarddesignrules.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardDesignRules::BoardDesignRules() noexcept
{
    restoreDefaults();
}

BoardDesignRules::BoardDesignRules(const BoardDesignRules& other)
{
    *this = other;
}

BoardDesignRules::BoardDesignRules(const SExpression& node) :
    BoardDesignRules() // this loads all default values!
{
    // general attributes
    mName = node.getValueByPath<QString>("name", true);
    mDescription = node.getValueByPath<QString>("description");
    // stop mask
    if (const SExpression* e = node.tryGetChildByPath("stopmask_clearance_ratio")) {
        mStopMaskClearanceRatio = e->getValueOfFirstChild<Ratio>();
    }
    if (const SExpression* e = node.tryGetChildByPath("stopmask_clearance_min")) {
        mStopMaskClearanceMin = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("stopmask_clearance_max")) {
        mStopMaskClearanceMax = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("stopmask_max_via_diameter")) {
        mStopMaskMaxViaDrillDiameter = e->getValueOfFirstChild<Length>();
    }
    // cream mask
    if (const SExpression* e = node.tryGetChildByPath("creammask_clearance_ratio")) {
        mCreamMaskClearanceRatio = e->getValueOfFirstChild<Ratio>();
    }
    if (const SExpression* e = node.tryGetChildByPath("creammask_clearance_min")) {
        mCreamMaskClearanceMin = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("creammask_clearance_max")) {
        mCreamMaskClearanceMax = e->getValueOfFirstChild<Length>();
    }
    // restring
    if (const SExpression* e = node.tryGetChildByPath("restring_pad_ratio")) {
        mRestringPadRatio = e->getValueOfFirstChild<Ratio>();
    }
    if (const SExpression* e = node.tryGetChildByPath("restring_pad_min")) {
        mRestringPadMin = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("restring_pad_max")) {
        mRestringPadMax = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("restring_via_ratio")) {
        mRestringViaRatio = e->getValueOfFirstChild<Ratio>();
    }
    if (const SExpression* e = node.tryGetChildByPath("restring_via_min")) {
        mRestringViaMin = e->getValueOfFirstChild<Length>();
    }
    if (const SExpression* e = node.tryGetChildByPath("restring_via_max")) {
        mRestringViaMax = e->getValueOfFirstChild<Length>();
    }
}

BoardDesignRules::~BoardDesignRules() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardDesignRules::restoreDefaults() noexcept
{
    // general attributes
    mName = tr("LibrePCB Default Design Rules");
    mDescription = QString();
    // stop mask
    mStopMaskClearanceRatio = Ratio(0);             // 0%
    mStopMaskClearanceMin = Length(100000);         // 0.1mm
    mStopMaskClearanceMax = Length(100000);         // 0.1mm
    mStopMaskMaxViaDrillDiameter = Length(500000);  // 0.5mm
    // cream mask
    mCreamMaskClearanceRatio = Ratio(100000);       // 10%
    mCreamMaskClearanceMin = Length(0);             // 0.0mm
    mCreamMaskClearanceMax = Length(1000000);       // 1.0mm
    // restring
    mRestringPadRatio = Ratio(250000);              // 25%
    mRestringPadMin = Length(250000);               // 0.25mm
    mRestringPadMax = Length(2000000);              // 2.0mm
    mRestringViaRatio = Ratio(250000);              // 25%
    mRestringViaMin = Length(200000);               // 0.2mm
    mRestringViaMax = Length(2000000);              // 2.0mm
}

void BoardDesignRules::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    // general attributes
    root.appendChild("name",                                mName, true);
    root.appendChild("description",                         mDescription, true);
    // stop mask
    root.appendChild("stopmask_clearance_ratio",            mStopMaskClearanceRatio, true);
    root.appendChild("stopmask_clearance_min",              mStopMaskClearanceMin, true);
    root.appendChild("stopmask_clearance_max",              mStopMaskClearanceMax, true);
    root.appendChild("stopmask_max_via_drill_diameter",     mStopMaskMaxViaDrillDiameter, true);
    // cream mask
    root.appendChild("creammask_clearance_ratio",           mCreamMaskClearanceRatio, true);
    root.appendChild("creammask_clearance_min",             mCreamMaskClearanceMin, true);
    root.appendChild("creammask_clearance_max",             mCreamMaskClearanceMax, true);
    // restring
    root.appendChild("restring_pad_ratio",                  mRestringPadRatio, true);
    root.appendChild("restring_pad_min",                    mRestringPadMin, true);
    root.appendChild("restring_pad_max",                    mRestringPadMax, true);
    root.appendChild("restring_via_ratio",                  mRestringViaRatio, true);
    root.appendChild("restring_via_min",                    mRestringViaMin, true);
    root.appendChild("restring_via_max",                    mRestringViaMax, true);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool BoardDesignRules::doesViaRequireStopMask(const Length& drillDia) const noexcept
{
    return (drillDia > mStopMaskMaxViaDrillDiameter ? true : false);
}

Length BoardDesignRules::calcStopMaskClearance(const Length& padSize) const noexcept
{
    return qBound(mStopMaskClearanceMin,
                  padSize.scaled(mStopMaskClearanceRatio.toNormalized()),
                  mStopMaskClearanceMax);
}

Length BoardDesignRules::calcCreamMaskClearance(const Length& padSize) const noexcept
{
    return qBound(mCreamMaskClearanceMin,
                  padSize.scaled(mCreamMaskClearanceRatio.toNormalized()),
                  mCreamMaskClearanceMax);
}

Length BoardDesignRules::calcPadRestring(const Length& drillDia) const noexcept
{
    return qBound(mRestringPadMin,
                  drillDia.scaled(mRestringPadRatio.toNormalized()),
                  mRestringPadMax);
}

Length BoardDesignRules::calcViaRestring(const Length& drillDia) const noexcept
{
    return qBound(mRestringViaMin,
                  drillDia.scaled(mRestringViaRatio.toNormalized()),
                  mRestringViaMax);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

BoardDesignRules& BoardDesignRules::operator=(const BoardDesignRules& rhs) noexcept
{
    // general attributes
    mName                           = rhs.mName;
    mDescription                    = rhs.mDescription;
    // stop mask
    mStopMaskClearanceRatio         = rhs.mStopMaskClearanceRatio;
    mStopMaskClearanceMin           = rhs.mStopMaskClearanceMin;
    mStopMaskClearanceMax           = rhs.mStopMaskClearanceMax;
    mStopMaskMaxViaDrillDiameter    = rhs.mStopMaskMaxViaDrillDiameter;
    // cream mask
    mCreamMaskClearanceRatio        = rhs.mCreamMaskClearanceRatio;
    mCreamMaskClearanceMin          = rhs.mCreamMaskClearanceMin;
    mCreamMaskClearanceMax          = rhs.mCreamMaskClearanceMax;
    // restring
    mRestringPadRatio               = rhs.mRestringPadRatio;
    mRestringPadMin                 = rhs.mRestringPadMin;
    mRestringPadMax                 = rhs.mRestringPadMax;
    mRestringViaRatio               = rhs.mRestringViaRatio;
    mRestringViaMin                 = rhs.mRestringViaMin;
    mRestringViaMax                 = rhs.mRestringViaMax;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BoardDesignRules::checkAttributesValidity() const noexcept
{
    // general attributes
    if (mName.isEmpty())                                    return false;
    // stop mask
    if (mStopMaskClearanceRatio < 0)                        return false;
    if (mStopMaskClearanceMin < 0)                          return false;
    if (mStopMaskClearanceMax < mStopMaskClearanceMin)      return false;
    if (mStopMaskMaxViaDrillDiameter < 0)                   return false;
    // cream mask
    if (mCreamMaskClearanceRatio < 0)                       return false;
    if (mCreamMaskClearanceMin < 0)                         return false;
    if (mCreamMaskClearanceMax < mCreamMaskClearanceMin)    return false;
    // restring
    if (mRestringPadRatio < 0)                              return false;
    if (mRestringPadMin < 0)                                return false;
    if (mRestringPadMax < mRestringPadMin)                  return false;
    if (mRestringViaRatio < 0)                              return false;
    if (mRestringViaMin < 0)                                return false;
    if (mRestringViaMax < mRestringViaMin)                  return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
