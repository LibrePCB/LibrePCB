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
#include "componentpinsignalmap.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const ComponentPinSignalMapItem& other) noexcept
{
    *this = other; // use assignment operator
}

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const Uuid& pin, const Uuid& signal,
                                                     const CmpSigPinDisplayType& displayType) noexcept :
    mPinUuid(pin), mSignalUuid(signal), mDisplayType(displayType)
{
}

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const SExpression& node)
{
    // read attributes
    mPinUuid = node.getChildByIndex(0).getValue<Uuid>();
    mDisplayType = CmpSigPinDisplayType::fromString(
                       node.getValueByPath<QString>("disp"));
    mSignalUuid = node.getValueByPath<Uuid>("sig");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentPinSignalMapItem::~ComponentPinSignalMapItem() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentPinSignalMapItem::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mPinUuid);
    root.appendChild("sig", mSignalUuid, false);
    root.appendChild("disp", mDisplayType, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool ComponentPinSignalMapItem::operator==(const ComponentPinSignalMapItem& rhs) const noexcept
{
    if (mPinUuid != rhs.mPinUuid)           return false;
    if (mSignalUuid != rhs.mSignalUuid)     return false;
    if (mDisplayType != rhs.mDisplayType)   return false;
    return true;
}

ComponentPinSignalMapItem& ComponentPinSignalMapItem::operator=(const ComponentPinSignalMapItem& rhs) noexcept
{
    mPinUuid = rhs.mPinUuid;
    mSignalUuid = rhs.mSignalUuid;
    mDisplayType = rhs.mDisplayType;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentPinSignalMapItem::checkAttributesValidity() const noexcept
{
    if (mPinUuid.isNull())  return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
