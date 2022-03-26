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

#include "../types/version.h"

#include <QtCore>

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
    mMirrored(other.mMirrored) {
}

NetLabel::NetLabel(const Uuid& uuid, const NetLabel& other) noexcept
  : NetLabel(other) {
  mUuid = uuid;
}

NetLabel::NetLabel(const Uuid& uuid, const Point& position,
                   const Angle& rotation, bool mirrored) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mPosition(position),
    mRotation(rotation),
    mMirrored(mirrored) {
}

NetLabel::NetLabel(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
    mMirrored(false) {
  if (fileFormat >= Version::fromString("0.2")) {
    mMirrored = deserialize<bool>(node.getChild("mirror/@0"), fileFormat);
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

bool NetLabel::setMirrored(const bool mirrored) noexcept {
  if (mirrored == mMirrored) {
    return false;
  }

  mMirrored = mirrored;
  onEdited.notify(Event::MirroredChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetLabel::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild(mPosition.serializeToDomElement("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("mirror", mMirrored);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool NetLabel::operator==(const NetLabel& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mMirrored != rhs.mMirrored) return false;
  return true;
}

NetLabel& NetLabel::operator=(const NetLabel& rhs) noexcept {
  setUuid(rhs.mUuid);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setMirrored(rhs.mMirrored);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
