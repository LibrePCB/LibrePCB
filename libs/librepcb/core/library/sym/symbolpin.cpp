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

#include "symbolpingraphicsitem.h"

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
    mRegisteredGraphicsItem(nullptr) {
}

SymbolPin::SymbolPin(const Uuid& uuid, const CircuitIdentifier& name,
                     const Point& position, const UnsignedLength& length,
                     const Angle& rotation) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mName(name),
    mPosition(position),
    mLength(length),
    mRotation(rotation),
    mRegisteredGraphicsItem(nullptr) {
}

SymbolPin::SymbolPin(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mName(deserialize<CircuitIdentifier>(node.getChild("name/@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat),
    mLength(
        deserialize<UnsignedLength>(node.getChild("length/@0"), fileFormat)),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
    mRegisteredGraphicsItem(nullptr) {
}

SymbolPin::~SymbolPin() noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool SymbolPin::setName(const CircuitIdentifier& name) noexcept {
  if (name == mName) {
    return false;
  }

  mName = name;
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setName(mName);
  onEdited.notify(Event::NameChanged);
  return true;
}

bool SymbolPin::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setPosition(mPosition);
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool SymbolPin::setLength(const UnsignedLength& length) noexcept {
  if (length == mLength) {
    return false;
  }

  mLength = length;
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setLength(mLength);
  onEdited.notify(Event::LengthChanged);
  return true;
}

bool SymbolPin::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setRotation(mRotation);
  onEdited.notify(Event::RotationChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPin::registerGraphicsItem(SymbolPinGraphicsItem& item) noexcept {
  Q_ASSERT(!mRegisteredGraphicsItem);
  mRegisteredGraphicsItem = &item;
}

void SymbolPin::unregisterGraphicsItem(SymbolPinGraphicsItem& item) noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == &item);
  mRegisteredGraphicsItem = nullptr;
}

void SymbolPin::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild(mPosition.serializeToDomElement("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("length", mLength);
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
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
