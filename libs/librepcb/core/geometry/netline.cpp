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

NetLineAnchor::NetLineAnchor(const tl::optional<Uuid>& junction,
                             const tl::optional<PinAnchor>& pin) noexcept
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

NetLineAnchor& NetLineAnchor::operator=(const NetLineAnchor& rhs) noexcept {
  mJunction = rhs.mJunction;
  mPin = rhs.mPin;
  return *this;
}

NetLineAnchor NetLineAnchor::junction(const Uuid& junction) noexcept {
  return NetLineAnchor(junction, tl::nullopt);
}

NetLineAnchor NetLineAnchor::pin(const Uuid& symbol, const Uuid& pin) noexcept {
  return NetLineAnchor(tl::nullopt, PinAnchor{symbol, pin});
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetLine::NetLine(const NetLine& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mWidth(other.mWidth),
    mStart(other.mStart),
    mEnd(other.mEnd) {
}

NetLine::NetLine(const Uuid& uuid, const NetLine& other) noexcept
  : NetLine(other) {
  mUuid = uuid;
}

NetLine::NetLine(const Uuid& uuid, const UnsignedLength& width,
                 const NetLineAnchor& start, const NetLineAnchor& end) noexcept
  : onEdited(*this), mUuid(uuid), mWidth(width), mStart(start), mEnd(end) {
}

NetLine::NetLine(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mWidth(deserialize<UnsignedLength>(node.getChild("width/@0"))),
    mStart(node.getChild("from")),
    mEnd(node.getChild("to")) {
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

bool NetLine::setStartPoint(const NetLineAnchor& start) noexcept {
  if (start == mStart) {
    return false;
  }

  mStart = start;
  onEdited.notify(Event::StartPointChanged);
  return true;
}

bool NetLine::setEndPoint(const NetLineAnchor& end) noexcept {
  if (end == mEnd) {
    return false;
  }

  mEnd = end;
  onEdited.notify(Event::EndPointChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetLine::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("width", mWidth);
  root.ensureLineBreak();
  mStart.serialize(root.appendList("from"));
  root.ensureLineBreak();
  mEnd.serialize(root.appendList("to"));
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool NetLine::operator==(const NetLine& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mStart != rhs.mStart) return false;
  if (mEnd != rhs.mEnd) return false;
  return true;
}

NetLine& NetLine::operator=(const NetLine& rhs) noexcept {
  setUuid(rhs.mUuid);
  setWidth(rhs.mWidth);
  setStartPoint(rhs.mStart);
  setEndPoint(rhs.mEnd);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
