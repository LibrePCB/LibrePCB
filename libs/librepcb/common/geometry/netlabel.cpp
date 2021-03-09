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
#include "netlabel.h"

#include <QtCore>

#include <version.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetLabel::NetLabel(const NetLabel& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mAlignment(other.mAlignment) {
}

NetLabel::NetLabel(const Uuid& uuid, const NetLabel& other) noexcept
  : NetLabel(other) {
  mUuid = uuid;
}

NetLabel::NetLabel(const Uuid& uuid, const Point& position,
                   const Angle& rotation, const Alignment& alignment) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mPosition(position),
    mRotation(rotation),
    mAlignment(alignment) {
}

NetLabel::NetLabel(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
    mAlignment() {
  if (fileFormat >= Version::fromString("0.2")) {
    Alignment tempAligment(node.getChild("alignment"), fileFormat);
    mAlignment = tempAligment;
  }
}

NetLabel::~NetLabel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool NetLabel::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool NetLabel::setPosition(const Point& position) noexcept {
  if (position == mPosition) {
    return false;
  }

  mPosition = position;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool NetLabel::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool NetLabel::setAlignment(const Alignment& alignment) noexcept {
  if (alignment == mAlignment) {
    return false;
  }

  mAlignment = alignment;
  onEdited.notify(Event::AlignmentChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetLabel::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild(mPosition.serializeToDomElement("position"), true);
  root.appendChild("rotation", mRotation, false);
  root.appendChild(mAlignment.serializeToDomElement("alignment"), false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool NetLabel::operator==(const NetLabel& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mAlignment != rhs.mAlignment) return false;
  return true;
}

NetLabel& NetLabel::operator=(const NetLabel& rhs) noexcept {
  setUuid(rhs.mUuid);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setAlignment(rhs.mAlignment);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
