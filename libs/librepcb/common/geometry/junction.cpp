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
#include "junction.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Junction::Junction(const Junction& other) noexcept
  : onEdited(*this), mUuid(other.mUuid), mPosition(other.mPosition) {
}

Junction::Junction(const Uuid& uuid, const Junction& other) noexcept
  : Junction(other) {
  mUuid = uuid;
}

Junction::Junction(const Uuid& uuid, const Point& position) noexcept
  : onEdited(*this), mUuid(uuid), mPosition(position) {
}

Junction::Junction(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat) {
}

Junction::~Junction() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Junction::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool Junction::setPosition(const Point& position) noexcept {
  if (position == mPosition) {
    return false;
  }

  mPosition = position;
  onEdited.notify(Event::PositionChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Junction::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Junction::operator==(const Junction& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  return true;
}

Junction& Junction::operator=(const Junction& rhs) noexcept {
  setUuid(rhs.mUuid);
  setPosition(rhs.mPosition);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
