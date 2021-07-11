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
#include "trace.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TraceAnchor
 ******************************************************************************/

TraceAnchor::TraceAnchor(const tl::optional<Uuid>& junction,
                         const tl::optional<Uuid>& via,
                         const tl::optional<PadAnchor>& pad) noexcept
  : mJunction(junction), mVia(via), mPad(pad) {
  Q_ASSERT(((junction) && (!via) && (!pad)) ||
           ((!junction) && (via) && (!pad)) ||
           ((!junction) && (!via) && (pad)));
}

TraceAnchor::TraceAnchor(const TraceAnchor& other) noexcept
  : mJunction(other.mJunction), mVia(other.mVia), mPad(other.mPad) {
}

TraceAnchor::TraceAnchor(const SExpression& node, const Version& fileFormat) {
  if (const SExpression* junctionNode = node.tryGetChild("junction")) {
    mJunction = deserialize<Uuid>(junctionNode->getChild("@0"), fileFormat);
  } else if (const SExpression* viaNode = node.tryGetChild("via")) {
    mVia = deserialize<Uuid>(viaNode->getChild("@0"), fileFormat);
  } else {
    mPad = PadAnchor{deserialize<Uuid>(node.getChild("device/@0"), fileFormat),
                     deserialize<Uuid>(node.getChild("pad/@0"), fileFormat)};
  }
}

TraceAnchor::~TraceAnchor() noexcept {
}

void TraceAnchor::serialize(SExpression& root) const {
  if (mJunction) {
    root.appendChild("junction", *mJunction, false);
  } else if (mVia) {
    root.appendChild("via", *mVia, false);
  } else if (mPad) {
    root.appendChild("device", mPad->device, false);
    root.appendChild("pad", mPad->pad, false);
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

bool TraceAnchor::operator==(const TraceAnchor& rhs) const noexcept {
  return (mJunction == rhs.mJunction) && (mVia == rhs.mVia) &&
      (mPad == rhs.mPad);
}

TraceAnchor& TraceAnchor::operator=(const TraceAnchor& rhs) noexcept {
  mJunction = rhs.mJunction;
  mVia = rhs.mVia;
  mPad = rhs.mPad;
  return *this;
}

TraceAnchor TraceAnchor::junction(const Uuid& junction) noexcept {
  return TraceAnchor(junction, tl::nullopt, tl::nullopt);
}

TraceAnchor TraceAnchor::via(const Uuid& via) noexcept {
  return TraceAnchor(tl::nullopt, via, tl::nullopt);
}

TraceAnchor TraceAnchor::pad(const Uuid& device, const Uuid& pad) noexcept {
  return TraceAnchor(tl::nullopt, tl::nullopt, PadAnchor{device, pad});
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Trace::Trace(const Trace& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mLayer(other.mLayer),
    mWidth(other.mWidth),
    mStart(other.mStart),
    mEnd(other.mEnd) {
}

Trace::Trace(const Uuid& uuid, const Trace& other) noexcept : Trace(other) {
  mUuid = uuid;
}

Trace::Trace(const Uuid& uuid, const GraphicsLayerName& layer,
             const PositiveLength& width, const TraceAnchor& start,
             const TraceAnchor& end) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayer(layer),
    mWidth(width),
    mStart(start),
    mEnd(end) {
}

Trace::Trace(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mLayer(
        deserialize<GraphicsLayerName>(node.getChild("layer/@0"), fileFormat)),
    mWidth(deserialize<PositiveLength>(node.getChild("width/@0"), fileFormat)),
    mStart(node.getChild("from"), fileFormat),
    mEnd(node.getChild("to"), fileFormat) {
}

Trace::~Trace() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Trace::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool Trace::setLayer(const GraphicsLayerName& layer) noexcept {
  if (layer == mLayer) {
    return false;
  }

  mLayer = layer;
  onEdited.notify(Event::LayerChanged);
  return true;
}

bool Trace::setWidth(const PositiveLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool Trace::setStartPoint(const TraceAnchor& start) noexcept {
  if (start == mStart) {
    return false;
  }

  mStart = start;
  onEdited.notify(Event::StartPointChanged);
  return true;
}

bool Trace::setEndPoint(const TraceAnchor& end) noexcept {
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

void Trace::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", SExpression::createToken(*mLayer), false);
  root.appendChild("width", mWidth, false);
  root.appendChild(mStart.serializeToDomElement("from"), true);
  root.appendChild(mEnd.serializeToDomElement("to"), true);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Trace::operator==(const Trace& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mStart != rhs.mStart) return false;
  if (mEnd != rhs.mEnd) return false;
  return true;
}

Trace& Trace::operator=(const Trace& rhs) noexcept {
  setUuid(rhs.mUuid);
  setLayer(rhs.mLayer);
  setWidth(rhs.mWidth);
  setStartPoint(rhs.mStart);
  setEndPoint(rhs.mEnd);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
