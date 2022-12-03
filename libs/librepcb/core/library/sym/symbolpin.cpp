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
#include "symbolpin.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolPin::SymbolPin(const SymbolPin& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mName(other.mName),
    mPosition(other.mPosition),
    mLength(other.mLength),
    mRotation(other.mRotation),
    mNamePosition(other.mNamePosition),
    mNameRotation(other.mNameRotation),
    mNameHeight(other.mNameHeight),
    mNameAlignment(other.mNameAlignment) {
}

SymbolPin::SymbolPin(const Uuid& uuid, const CircuitIdentifier& name,
                     const Point& position, const UnsignedLength& length,
                     const Angle& rotation, const Point& namePosition,
                     const Angle& nameRotation,
                     const PositiveLength& nameHeight,
                     const Alignment& nameAlign) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mName(name),
    mPosition(position),
    mLength(length),
    mRotation(rotation),
    mNamePosition(namePosition),
    mNameRotation(nameRotation),
    mNameHeight(nameHeight),
    mNameAlignment(nameAlign) {
}

SymbolPin::SymbolPin(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mName(deserialize<CircuitIdentifier>(node.getChild("name/@0"))),
    mPosition(node.getChild("position")),
    mLength(deserialize<UnsignedLength>(node.getChild("length/@0"))),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mNamePosition(node.getChild("name_position")),
    mNameRotation(deserialize<Angle>(node.getChild("name_rotation/@0"))),
    mNameHeight(deserialize<PositiveLength>(node.getChild("name_height/@0"))),
    mNameAlignment(node.getChild("name_align")) {
}

SymbolPin::~SymbolPin() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool SymbolPin::setName(const CircuitIdentifier& name) noexcept {
  if (name == mName) {
    return false;
  }

  mName = name;
  onEdited.notify(Event::NameChanged);
  return true;
}

bool SymbolPin::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool SymbolPin::setLength(const UnsignedLength& length) noexcept {
  if (length == mLength) {
    return false;
  }

  mLength = length;
  onEdited.notify(Event::LengthChanged);
  return true;
}

bool SymbolPin::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool SymbolPin::setNamePosition(const Point& position) noexcept {
  if (position == mNamePosition) {
    return false;
  }

  mNamePosition = position;
  onEdited.notify(Event::NamePositionChanged);
  return true;
}

bool SymbolPin::setNameRotation(const Angle& rotation) noexcept {
  if (rotation == mNameRotation) {
    return false;
  }

  mNameRotation = rotation;
  onEdited.notify(Event::NameRotationChanged);
  return true;
}

bool SymbolPin::setNameHeight(const PositiveLength& height) noexcept {
  if (height == mNameHeight) {
    return false;
  }

  mNameHeight = height;
  onEdited.notify(Event::NameHeightChanged);
  return true;
}

bool SymbolPin::setNameAlignment(const Alignment& align) noexcept {
  if (align == mNameAlignment) {
    return false;
  }

  mNameAlignment = align;
  onEdited.notify(Event::NameAlignmentChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPin::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("length", mLength);
  root.ensureLineBreak();
  mNamePosition.serialize(root.appendList("name_position"));
  root.appendChild("name_rotation", mNameRotation);
  root.appendChild("name_height", mNameHeight);
  root.ensureLineBreak();
  mNameAlignment.serialize(root.appendList("name_align"));
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool SymbolPin::operator==(const SymbolPin& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mName != rhs.mName) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mLength != rhs.mLength) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mNamePosition != rhs.mNamePosition) return false;
  if (mNameRotation != rhs.mNameRotation) return false;
  if (mNameHeight != rhs.mNameHeight) return false;
  if (mNameAlignment != rhs.mNameAlignment) return false;
  return true;
}

SymbolPin& SymbolPin::operator=(const SymbolPin& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setName(rhs.mName);
  setPosition(rhs.mPosition);
  setLength(rhs.mLength);
  setRotation(rhs.mRotation);
  setNamePosition(rhs.mNamePosition);
  setNameRotation(rhs.mNameRotation);
  setNameHeight(rhs.mNameHeight);
  setNameAlignment(rhs.mNameAlignment);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
