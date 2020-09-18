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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GridProperties::GridProperties() noexcept
  : mType(Type_t::Lines), mInterval(2540000), mUnit(LengthUnit::millimeters()) {
}

GridProperties::GridProperties(const SExpression& node)
  : mType(node.getValueByPath<Type_t>("type")),
    mInterval(node.getValueByPath<PositiveLength>("interval")),
    mUnit(node.getValueByPath<LengthUnit>("unit")) {
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
  root.appendChild("type", mType, false);
  root.appendChild("interval", mInterval, false);
  root.appendChild("unit", mUnit, false);
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
