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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPin::SymbolPin(const Uuid& uuid, const QString& name, const Point& position,
                     const Length& length, const Angle& rotation) noexcept :
    mUuid(uuid), mName(name), mPosition(position), mLength(length), mRotation(rotation)
{
    Q_ASSERT(!mUuid.isNull());
    Q_ASSERT(!mName.isEmpty());
    Q_ASSERT(mLength >= 0);
}

SymbolPin::SymbolPin(const DomElement& domElement) throw (Exception) :
    mUuid(), mPosition(), mLength(), mRotation()
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getText<QString>(true); // empty string --> exception
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mLength = domElement.getAttribute<Length>("length", true);
    mRotation = domElement.getAttribute<Angle>("rotation", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolPin::~SymbolPin() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SymbolPin::setName(const QString& name) noexcept
{
    Q_ASSERT(!name.isEmpty());
    mName = name;
}

void SymbolPin::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
}

void SymbolPin::setLength(const Length& length) noexcept
{
    Q_ASSERT(length >= 0);
    mLength = length;
}

void SymbolPin::setRotation(const Angle& rotation) noexcept
{
    mRotation = rotation;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPin::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("x", mPosition.getX());
    root.setAttribute("y", mPosition.getY());
    root.setAttribute("length", mLength);
    root.setAttribute("rotation", mRotation);
    root.setText(mName);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPin::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mLength < 0)        return false;
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
