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
#include "hole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Hole::Hole(const Hole& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mPosition(other.mPosition),
    mDiameter(other.mDiameter) {
}

Hole::Hole(const Uuid& uuid, const Hole& other) noexcept : Hole(other) {
  mUuid = uuid;
}

Hole::Hole(const Uuid& uuid, const Point& position,
           const PositiveLength& diameter) noexcept
  : onEdited(*this), mUuid(uuid), mPosition(position), mDiameter(diameter) {
}

Hole::Hole(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat),
    mDiameter(
        deserialize<PositiveLength>(node.getChild("diameter/@0"), fileFormat)) {
}

Hole::~Hole() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Hole::setPosition(const Point& position) noexcept {
  if (position == mPosition) {
    return false;
  }

  mPosition = position;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool Hole::setDiameter(const PositiveLength& diameter) noexcept {
  if (diameter == mDiameter) {
    return false;
  }

  mDiameter = diameter;
  onEdited.notify(Event::DiameterChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Hole::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("diameter", mDiameter, false);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Hole::operator==(const Hole& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mDiameter != rhs.mDiameter) return false;
  return true;
}

Hole& Hole::operator=(const Hole& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setPosition(rhs.mPosition);
  setDiameter(mDiameter = rhs.mDiameter);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
