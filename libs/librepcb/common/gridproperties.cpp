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
#include "gridproperties.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GridProperties::GridProperties() noexcept :
    mType(Type_t::Lines), mInterval(2540000), mUnit(LengthUnit::millimeters())
{
}

GridProperties::GridProperties(const DomElement& domElement) throw (Exception)
{
    mType = stringToType(domElement.getAttribute<QString>("type", true));
    mInterval = domElement.getAttribute<Length>("interval", true);
    mUnit = domElement.getAttribute<LengthUnit>("unit", true);
}

GridProperties::GridProperties(Type_t type, const Length& interval, const LengthUnit& unit) noexcept :
    mType(type), mInterval(interval), mUnit(unit)
{
}

GridProperties::GridProperties(const GridProperties& other) noexcept :
    mType(other.mType), mInterval(other.mInterval), mUnit(other.mUnit)
{
}

GridProperties::~GridProperties() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GridProperties::serialize(DomElement& root) const throw (Exception)
{
    root.setAttribute("type", typeToString(mType));
    root.setAttribute("interval", mInterval);
    root.setAttribute("unit", mUnit);
}

/*****************************************************************************************
 *  Operators
 ****************************************************************************************/

GridProperties& GridProperties::operator=(const GridProperties& rhs) noexcept
{
    mType = rhs.mType;
    mInterval = rhs.mInterval;
    mUnit = rhs.mUnit;
    return *this;
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

GridProperties::Type_t GridProperties::stringToType(const QString& type) throw (Exception)
{
    if (type == "off")          return Type_t::Off;
    else if (type == "lines")   return Type_t::Lines;
    else if (type == "dots")    return Type_t::Dots;
    else throw RuntimeError(__FILE__, __LINE__, QString(tr("Unknown grid type: \"%1\"")).arg(type));
}

QString GridProperties::typeToString(Type_t type) throw (Exception)
{
    switch (type)
    {
        case Type_t::Off:   return "off";
        case Type_t::Lines: return "lines";
        case Type_t::Dots:  return "dots";
        default: throw LogicError(__FILE__, __LINE__);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
