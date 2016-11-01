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
#include "fileio/xmldomelement.h"

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

BoardDesignRules::BoardDesignRules(const XmlDomElement& domElement) throw (Exception) :
    BoardDesignRules() // this loads all default values!
{
    // general attributes
    mName = domElement.getFirstChild("name", true)->getText<QString>(true);
    mDescription = domElement.getFirstChild("description", true)->getText<QString>(false);
    // stop mask
    if (XmlDomElement* e = domElement.getFirstChild("stopmask_clearance_ratio", false)) {
        mStopMaskClearanceRatio = e->getText<qreal>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("stopmask_clearance_min", false)) {
        mStopMaskClearanceMin = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("stopmask_clearance_max", false)) {
        mStopMaskClearanceMax = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("stopmask_max_via_diameter", false)) {
        mStopMaskMaxViaDrillDiameter = e->getText<Length>(true);
    }
    // cream mask
    if (XmlDomElement* e = domElement.getFirstChild("creammask_clearance_ratio", false)) {
        mCreamMaskClearanceRatio = e->getText<qreal>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("creammask_clearance_min", false)) {
        mCreamMaskClearanceMin = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("creammask_clearance_max", false)) {
        mCreamMaskClearanceMax = e->getText<Length>(true);
    }
    // restring
    if (XmlDomElement* e = domElement.getFirstChild("restring_pad_ratio", false)) {
        mRestringPadRatio = e->getText<qreal>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("restring_pad_min", false)) {
        mRestringPadMin = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("restring_pad_max", false)) {
        mRestringPadMax = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("restring_via_ratio", false)) {
        mRestringViaRatio = e->getText<qreal>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("restring_via_min", false)) {
        mRestringViaMin = e->getText<Length>(true);
    }
    if (XmlDomElement* e = domElement.getFirstChild("restring_via_max", false)) {
        mRestringViaMax = e->getText<Length>(true);
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
    mStopMaskClearanceRatio = qreal(0);             // 0%
    mStopMaskClearanceMin = Length(100000);         // 0.1mm
    mStopMaskClearanceMax = Length(100000);         // 0.1mm
    mStopMaskMaxViaDrillDiameter = Length(500000);  // 0.5mm
    // cream mask
    mCreamMaskClearanceRatio = qreal(0.1);          // 10%
    mCreamMaskClearanceMin = Length(0);             // 0.0mm
    mCreamMaskClearanceMax = Length(1000000);       // 1.0mm
    // restring
    mRestringPadRatio = qreal(0.25);                // 25%
    mRestringPadMin = Length(250000);               // 0.25mm
    mRestringPadMax = Length(2000000);              // 2.0mm
    mRestringViaRatio = qreal(0.25);                // 25%
    mRestringViaMin = Length(200000);               // 0.2mm
    mRestringViaMax = Length(2000000);              // 2.0mm
}

XmlDomElement* BoardDesignRules::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("board_design_rules"));
    // general attributes
    root->appendTextChild("name",                               mName);
    root->appendTextChild("description",                        mDescription);
    // stop mask
    root->appendTextChild("stopmask_clearance_ratio",           mStopMaskClearanceRatio);
    root->appendTextChild("stopmask_clearance_min",             mStopMaskClearanceMin);
    root->appendTextChild("stopmask_clearance_max",             mStopMaskClearanceMax);
    root->appendTextChild("stopmask_max_via_drill_diameter",    mStopMaskMaxViaDrillDiameter);
    // cream mask
    root->appendTextChild("creammask_clearance_ratio",          mCreamMaskClearanceRatio);
    root->appendTextChild("creammask_clearance_min",            mCreamMaskClearanceMin);
    root->appendTextChild("creammask_clearance_max",            mCreamMaskClearanceMax);
    // restring
    root->appendTextChild("restring_pad_ratio",                 mRestringPadRatio);
    root->appendTextChild("restring_pad_min",                   mRestringPadMin);
    root->appendTextChild("restring_pad_max",                   mRestringPadMax);
    root->appendTextChild("restring_via_ratio",                 mRestringViaRatio);
    root->appendTextChild("restring_via_min",                   mRestringViaMin);
    root->appendTextChild("restring_via_max",                   mRestringViaMax);
    // end
    return root.take();
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
                  padSize.scaled(mStopMaskClearanceRatio),
                  mStopMaskClearanceMax);
}

Length BoardDesignRules::calcCreamMaskClearance(const Length& padSize) const noexcept
{
    return qBound(mCreamMaskClearanceMin,
                  padSize.scaled(mCreamMaskClearanceRatio),
                  mCreamMaskClearanceMax);
}

Length BoardDesignRules::calcPadRestring(const Length& drillDia) const noexcept
{
    return qBound(mRestringPadMin,
                  drillDia.scaled(mRestringPadRatio),
                  mRestringPadMax);
}

Length BoardDesignRules::calcViaRestring(const Length& drillDia) const noexcept
{
    return qBound(mRestringViaMin,
                  drillDia.scaled(mRestringViaRatio),
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
