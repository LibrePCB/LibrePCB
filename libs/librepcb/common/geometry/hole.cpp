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

Hole::Hole(const Point& position, const Length& diameter) noexcept :
    mPosition(position), mDiameter(diameter)
{
    Q_ASSERT(diameter > 0);
}

Hole::Hole(const DomElement& domElement) throw (Exception)
{
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mDiameter = domElement.getAttribute<Length>("diameter", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Hole::~Hole() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Hole::setPosition(const Point& position) noexcept
{
    mPosition = position;
}

void Hole::setDiameter(const Length& diameter) noexcept
{
    Q_ASSERT(diameter > 0);
    mDiameter = diameter;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Hole::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("x", mPosition.getX());
    root.setAttribute("y", mPosition.getY());
    root.setAttribute("diameter", mDiameter);
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
