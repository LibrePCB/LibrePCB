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
#include "netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NetLineAnchor
 ******************************************************************************/

NetLineAnchor::NetLineAnchor(const std::optional<Uuid>& junction,
                             const std::optional<PinAnchor>& pin) noexcept
  : mJunction(junction), mPin(pin) {
  Q_ASSERT(((junction) && (!pin)) || ((!junction) && (pin)));
}

NetLineAnchor::NetLineAnchor(const NetLineAnchor& other) noexcept
  : mJunction(other.mJunction), mPin(other.mPin) {
}

NetLineAnchor::NetLineAnchor(const SExpression& node) {
  if (const SExpression* junctionNode = node.tryGetChild("junction")) {
    mJunction = deserialize<Uuid>(junctionNode->getChild("@0"));
  } else {
    mPin = PinAnchor{deserialize<Uuid>(node.getChild("symbol/@0")),
                     deserialize<Uuid>(node.getChild("pin/@0"))};
  }
}

NetLineAnchor::~NetLineAnchor() noexcept {
}

void NetLineAnchor::serialize(SExpression& root) const {
  if (mJunction) {
    root.appendChild("junction", *mJunction);
  } else if (mPin) {
    root.appendChild("symbol", mPin->symbol);
    root.appendChild("pin", mPin->pin);
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

bool NetLineAnchor::operator==(const NetLineAnchor& rhs) const noexcept {
  return (mJunction == rhs.mJunction) && (mPin == rhs.mPin);
}

bool NetLineAnchor::operator<(const NetLineAnchor& rhs) const noexcept {
  // Note: This operator is relevant for the file format, do not modify
  // unless you know exactly what you're doing!
  if (mJunction.has_value() != rhs.mJunction.has_value()) {
    return rhs.mJunction.has_value();
  } else if (mPin.has_value() != rhs.mPin.has_value()) {
    return rhs.mPin.has_value();
  } else if (mJunction) {
    return (*mJunction) < (*rhs.mJunction);
  } else if (mPin) {
    if (mPin->symbol != rhs.mPin->symbol) {
      return mPin->symbol < rhs.mPin->symbol;
    } else {
      return mPin->pin < rhs.mPin->pin;
    }
  } else {
    qWarning() << "Unhandled branch in NetLineAnchor::operator<().";
    return false;
  }
}

NetLineAnchor& NetLineAnchor::operator=(const NetLineAnchor& rhs) noexcept {
  mJunction = rhs.mJunction;
  mPin = rhs.mPin;
  return *this;
}

NetLineAnchor NetLineAnchor::junction(const Uuid& junction) noexcept {
  return NetLineAnchor(junction, std::nullopt);
}

NetLineAnchor NetLineAnchor::pin(const Uuid& symbol, const Uuid& pin) noexcept {
  return NetLineAnchor(std::nullopt, PinAnchor{symbol, pin});
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetLine::NetLine(const NetLine& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mWidth(other.mWidth),
    mP1(other.mP1),
    mP2(other.mP2) {
}

NetLine::NetLine(const Uuid& uuid, const NetLine& other) noexcept
  : NetLine(other) {
  mUuid = uuid;
}

NetLine::NetLine(const Uuid& uuid, const UnsignedLength& width,
                 const NetLineAnchor& a, const NetLineAnchor& b) noexcept
  : onEdited(*this), mUuid(uuid), mWidth(width), mP1(a), mP2(b) {
  normalizeAnchors(mP1, mP2);
}

NetLine::NetLine(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mWidth(deserialize<UnsignedLength>(node.getChild("width/@0"))),
    mP1(node.getChild("from")),
    mP2(node.getChild("to")) {
  normalizeAnchors(mP1, mP2);
}

NetLine::~NetLine() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool NetLine::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool NetLine::setWidth(const UnsignedLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool NetLine::setAnchors(NetLineAnchor a, NetLineAnchor b) noexcept {
  normalizeAnchors(a, b);
  if ((a == mP1) && (b == mP2)) {
    return false;
  }

  mP1 = a;
  mP2 = b;
  onEdited.notify(Event::AnchorsChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetLine::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("width", mWidth);
  root.ensureLineBreak();
  mP1.serialize(root.appendList("from"));
  root.ensureLineBreak();
  mP2.serialize(root.appendList("to"));
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool NetLine::operator==(const NetLine& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mP1 != rhs.mP1) return false;
  if (mP2 != rhs.mP2) return false;
  return true;
}

NetLine& NetLine::operator=(const NetLine& rhs) noexcept {
  setUuid(rhs.mUuid);
  setWidth(rhs.mWidth);
  setAnchors(rhs.mP1, rhs.mP2);
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetLine::normalizeAnchors(NetLineAnchor& start,
                               NetLineAnchor& end) noexcept {
  if (end < start) {
    std::swap(start, end);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
