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
#include "symbolpin.h"
#include "symbolpingraphicsitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPin::SymbolPin(const SymbolPin& other) noexcept :
    mUuid(other.mUuid),
    mName(other.mName),
    mPosition(other.mPosition),
    mLength(other.mLength),
    mRotation(other.mRotation),
    mRegisteredGraphicsItem(nullptr)
{
}

SymbolPin::SymbolPin(const Uuid& uuid, const QString& name, const Point& position,
                     const Length& length, const Angle& rotation) noexcept :
    mUuid(uuid), mName(name), mPosition(position), mLength(length), mRotation(rotation),
    mRegisteredGraphicsItem(nullptr)
{
    Q_ASSERT(!mName.isEmpty());
    Q_ASSERT(mLength >= 0);
}

SymbolPin::SymbolPin(const SExpression& node) :
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mName(node.getValueByPath<QString>("name", true)),
    mPosition(node.getChildByPath("pos")),
    mLength(node.getValueByPath<Length>("length")),
    mRotation(node.getValueByPath<Angle>("rot")),
    mRegisteredGraphicsItem(nullptr)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolPin::~SymbolPin() noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SymbolPin::setName(const QString& name) noexcept
{
    Q_ASSERT(!name.isEmpty());
    mName = name;
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setName(mName);
}

void SymbolPin::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setPosition(mPosition);
}

void SymbolPin::setLength(const Length& length) noexcept
{
    Q_ASSERT(length >= 0);
    mLength = length;
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setLength(mLength);
}

void SymbolPin::setRotation(const Angle& rotation) noexcept
{
    mRotation = rotation;
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setRotation(mRotation);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPin::registerGraphicsItem(SymbolPinGraphicsItem& item) noexcept
{
    Q_ASSERT(!mRegisteredGraphicsItem);
    mRegisteredGraphicsItem = &item;
}

void SymbolPin::unregisterGraphicsItem(SymbolPinGraphicsItem& item) noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == &item);
    mRegisteredGraphicsItem = nullptr;
}

void SymbolPin::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("name", mName, false);
    root.appendChild(mPosition.serializeToDomElement("pos"), true);
    root.appendChild("rot", mRotation, false);
    root.appendChild("length", mLength, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool SymbolPin::operator==(const SymbolPin& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)         return false;
    if (mName != rhs.mName)         return false;
    if (mPosition != rhs.mPosition) return false;
    if (mLength != rhs.mLength)     return false;
    if (mRotation != rhs.mRotation) return false;
    return true;
}

SymbolPin& SymbolPin::operator=(const SymbolPin& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mName = rhs.mName;
    mPosition = rhs.mPosition;
    mLength = rhs.mLength;
    mRotation = rhs.mRotation;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPin::checkAttributesValidity() const noexcept
{
    if (mLength < 0)        return false;
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
