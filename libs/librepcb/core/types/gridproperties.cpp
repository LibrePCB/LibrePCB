/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "gridproperties.h"

#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const GridProperties::Type_t& obj) {
  switch (obj) {
    case GridProperties::Type_t::Off:
      return SExpression::createToken("off");
    case GridProperties::Type_t::Lines:
      return SExpression::createToken("lines");
    case GridProperties::Type_t::Dots:
      return SExpression::createToken("dots");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline GridProperties::Type_t deserialize(const SExpression& sexpr) {
  const QString str = sexpr.getValue();
  if (str == "off") {
    return GridProperties::Type_t::Off;
  } else if (str == "lines") {
    return GridProperties::Type_t::Lines;
  } else if (str == "dots") {
    return GridProperties::Type_t::Dots;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown grid type: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GridProperties::GridProperties() noexcept
  : mType(Type_t::Lines), mInterval(2540000), mUnit(LengthUnit::millimeters()) {
}

GridProperties::GridProperties(const SExpression& node)
  : mType(deserialize<Type_t>(node.getChild("type/@0"))),
    mInterval(deserialize<PositiveLength>(node.getChild("interval/@0"))),
    mUnit(deserialize<LengthUnit>(node.getChild("unit/@0"))) {
}

GridProperties::GridProperties(Type_t type, const PositiveLength& interval,
                               const LengthUnit& unit) noexcept
  : mType(type), mInterval(interval), mUnit(unit) {
}

GridProperties::GridProperties(const GridProperties& other) noexcept
  : mType(other.mType), mInterval(other.mInterval), mUnit(other.mUnit) {
}

GridProperties::~GridProperties() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GridProperties::serialize(SExpression& root) const {
  root.appendChild("type", mType);
  root.appendChild("interval", mInterval);
  root.appendChild("unit", mUnit);
}

/*******************************************************************************
 *  Operators
 ******************************************************************************/

GridProperties& GridProperties::operator=(const GridProperties& rhs) noexcept {
  mType = rhs.mType;
  mInterval = rhs.mInterval;
  mUnit = rhs.mUnit;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
