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
#include "devicepadsignalmap.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DevicePadSignalMapItem::DevicePadSignalMapItem(const DevicePadSignalMapItem& other) noexcept :
    QObject(nullptr),
    mPadUuid(other.mPadUuid),
    mSignalUuid(other.mSignalUuid)
{
}

DevicePadSignalMapItem::DevicePadSignalMapItem(const Uuid& pad, const tl::optional<Uuid>& signal) noexcept :
    QObject(nullptr), mPadUuid(pad), mSignalUuid(signal)
{
}

DevicePadSignalMapItem::DevicePadSignalMapItem(const SExpression& node) :
    QObject(nullptr),
    mPadUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mSignalUuid(node.getValueByPath<tl::optional<Uuid>>("sig"))
{
}

DevicePadSignalMapItem::~DevicePadSignalMapItem() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void DevicePadSignalMapItem::setSignalUuid(const tl::optional<Uuid>& uuid) noexcept
{
    if (uuid == mSignalUuid) return;
    mSignalUuid = uuid;
    emit signalUuidChanged(mSignalUuid);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void DevicePadSignalMapItem::serialize(SExpression& root) const
{
    root.appendChild(mPadUuid);
    root.appendChild("sig", mSignalUuid, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool DevicePadSignalMapItem::operator==(const DevicePadSignalMapItem& rhs) const noexcept
{
    if (mPadUuid != rhs.mPadUuid)           return false;
    if (mSignalUuid != rhs.mSignalUuid)     return false;
    return true;
}

DevicePadSignalMapItem& DevicePadSignalMapItem::operator=(const DevicePadSignalMapItem& rhs) noexcept
{
    mPadUuid = rhs.mPadUuid;
    setSignalUuid(rhs.mSignalUuid);
    return *this;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
