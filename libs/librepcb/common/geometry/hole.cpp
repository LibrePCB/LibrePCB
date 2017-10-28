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
#include "hole.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Hole::Hole(const Hole& other) noexcept :
    mPosition(other.mPosition), mDiameter(other.mDiameter)
{
}

Hole::Hole(const Point& position, const Length& diameter) noexcept :
    mPosition(position), mDiameter(diameter)
{
}

Hole::Hole(const SExpression& node)
{
    mPosition = Point(node.getChildByPath("pos"));
    mDiameter = node.getValueByPath<Length>("dia", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Hole::~Hole() noexcept
{
    Q_ASSERT(mObservers.isEmpty());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Hole::setPosition(const Point& position) noexcept
{
    if (position == mPosition) return;
    mPosition = position;
    foreach (IF_HoleObserver* object, mObservers) {
        object->holePositionChanged(mPosition);
    }
}

void Hole::setDiameter(const Length& diameter) noexcept
{
    if (diameter == mDiameter) return;
    mDiameter = diameter;
    foreach (IF_HoleObserver* object, mObservers) {
        object->holeDiameterChanged(mDiameter);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Hole::registerObserver(IF_HoleObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void Hole::unregisterObserver(IF_HoleObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void Hole::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mPosition.serializeToDomElement("pos"), false);
    root.appendTokenChild("dia", mDiameter, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Hole::operator==(const Hole& rhs) const noexcept
{
    if (mPosition != rhs.mPosition)             return false;
    if (mDiameter != rhs.mDiameter)             return false;
    return true;
}

Hole& Hole::operator=(const Hole& rhs) noexcept
{
    mPosition = rhs.mPosition;
    mDiameter = rhs.mDiameter;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Hole::checkAttributesValidity() const noexcept
{
    if (mDiameter <= 0)          return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
