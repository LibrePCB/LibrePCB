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
#include "componentpinsignalmapitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const Uuid& pin, const Uuid& signal,
                                                     PinDisplayType_t displayType) noexcept :
    mPinUuid(pin), mSignalUuid(signal), mDisplayType(displayType)
{
}

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const DomElement& domElement) throw (Exception)
{
    // read attributes
    mPinUuid = domElement.getAttribute<Uuid>("pin", true);
    mDisplayType = stringToDisplayType(domElement.getAttribute<QString>("display", true));
    mSignalUuid = domElement.getText<Uuid>(false);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentPinSignalMapItem::~ComponentPinSignalMapItem() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentPinSignalMapItem::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("pin", mPinUuid);
    root.setAttribute<QString>("display", displayTypeToString(mDisplayType));
    root.setText(mSignalUuid);
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
 *  Static Methods
 ****************************************************************************************/

ComponentPinSignalMapItem::PinDisplayType_t ComponentPinSignalMapItem::stringToDisplayType(const QString& type) throw (Exception)
{
    if      (type == QLatin1String("none"))             return PinDisplayType_t::NONE;
    else if (type == QLatin1String("pin_name"))         return PinDisplayType_t::PIN_NAME;
    else if (type == QLatin1String("component_signal")) return PinDisplayType_t::COMPONENT_SIGNAL;
    else if (type == QLatin1String("net_signal"))       return PinDisplayType_t::NET_SIGNAL;
    else throw RuntimeError(__FILE__, __LINE__, QString(), type);
}

QString ComponentPinSignalMapItem::displayTypeToString(PinDisplayType_t type) noexcept
{
    switch (type)
    {
        case PinDisplayType_t::NONE:                return QString("none");
        case PinDisplayType_t::PIN_NAME:            return QString("pin_name");
        case PinDisplayType_t::COMPONENT_SIGNAL:    return QString("component_signal");
        case PinDisplayType_t::NET_SIGNAL:          return QString("net_signal");
        default: Q_ASSERT(false);                   return QString();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
