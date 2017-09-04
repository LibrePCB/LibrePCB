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
#include "componentsymbolvariantitem.h"
#include "component.h"
#include "componentsymbolvariant.h"
#include "componentpinsignalmap.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const ComponentSymbolVariantItem& other) noexcept
{
    *this = other; // use assignment operator
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const Uuid& uuid,
        const Uuid& symbolUuid, bool isRequired, const QString& suffix) noexcept :
    mUuid(uuid), mSymbolUuid(symbolUuid), mIsRequired(isRequired), mSuffix(suffix)
{
    Q_ASSERT(mUuid.isNull() == false);
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const DomElement& domElement)
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mSymbolUuid = domElement.getAttribute<Uuid>("symbol", true);
    mSymbolPos.setX(domElement.getAttribute<Length>("x", true));
    mSymbolPos.setY(domElement.getAttribute<Length>("y", true));
    mSymbolRot = domElement.getAttribute<Angle>("rotation", true);
    mIsRequired = domElement.getAttribute<bool>("required", true);
    mSuffix = domElement.getAttribute<QString>("suffix", false);

    // read pin signal map
    mPinSignalMap.loadFromDomElement(domElement);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSymbolVariantItem::~ComponentSymbolVariantItem() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSymbolVariantItem::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("symbol", mSymbolUuid);
    root.setAttribute("x", mSymbolPos.getX());
    root.setAttribute("y", mSymbolPos.getY());
    root.setAttribute("rotation", mSymbolRot);
    root.setAttribute("required", mIsRequired);
    root.setAttribute("suffix", mSuffix);
    mPinSignalMap.serialize(root);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool ComponentSymbolVariantItem::operator==(const ComponentSymbolVariantItem& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                                     return false;
    if (mSymbolUuid != rhs.mSymbolUuid)                         return false;
    if (mIsRequired != rhs.mIsRequired)                         return false;
    if (mSuffix != rhs.mSuffix)                                 return false;
    if (mPinSignalMap != rhs.mPinSignalMap)                     return false;
    return true;
}

ComponentSymbolVariantItem& ComponentSymbolVariantItem::operator=(const ComponentSymbolVariantItem& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mSymbolUuid = rhs.mSymbolUuid;
    mSymbolPos = rhs.mSymbolPos;
    mSymbolRot = rhs.mSymbolRot;
    mIsRequired = rhs.mIsRequired;
    mSuffix = rhs.mSuffix;
    mPinSignalMap = rhs.mPinSignalMap;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSymbolVariantItem::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mSymbolUuid.isNull())               return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
