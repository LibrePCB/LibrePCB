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

TraceAnchor::TraceAnchor(const std::optional<Uuid>& junction,
                         const std::optional<Uuid>& via,
                         const std::optional<Uuid>& pad,
                         const std::optional<PadAnchor>& footprintPad) noexcept
  : mJunction(junction), mVia(via), mPad(pad), mFootprintPad(footprintPad) {
  Q_ASSERT(((junction) && (!via) && (!pad) && (!footprintPad)) ||
           ((!junction) && (via) && (!pad) && (!footprintPad)) ||
           ((!junction) && (!via) && (pad) && (!footprintPad)) ||
           ((!junction) && (!via) && (!pad) && (footprintPad)));
}

TraceAnchor::TraceAnchor(const TraceAnchor& other) noexcept
  : mJunction(other.mJunction),
    mVia(other.mVia),
    mPad(other.mPad),
    mFootprintPad(other.mFootprintPad) {
}

TraceAnchor::TraceAnchor(const SExpression& node) {
  if (const SExpression* junctionNode = node.tryGetChild("junction")) {
    mJunction = deserialize<Uuid>(junctionNode->getChild("@0"));
  } else if (const SExpression* viaNode = node.tryGetChild("via")) {
    mVia = deserialize<Uuid>(viaNode->getChild("@0"));
  } else if (const SExpression* devNode = node.tryGetChild("device")) {
    mFootprintPad = PadAnchor{deserialize<Uuid>(devNode->getChild("@0")),
                              deserialize<Uuid>(node.getChild("pad/@0"))};
  } else {
    mPad = deserialize<Uuid>(node.getChild("pad/@0"));
  }
}

TraceAnchor::~TraceAnchor() noexcept {
}

void TraceAnchor::serialize(SExpression& root) const {
  if (mJunction) {
    root.appendChild("junction", *mJunction);
  } else if (mVia) {
    root.appendChild("via", *mVia);
  } else if (mPad) {
    root.appendChild("pad", *mPad);
  } else if (mFootprintPad) {
    root.appendChild("device", mFootprintPad->device);
    root.appendChild("pad", mFootprintPad->pad);
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

bool TraceAnchor::operator==(const TraceAnchor& rhs) const noexcept {
  return (mJunction == rhs.mJunction) && (mVia == rhs.mVia) &&
      (mPad == rhs.mPad) && (mFootprintPad == rhs.mFootprintPad);
}

bool TraceAnchor::operator<(const TraceAnchor& rhs) const noexcept {
  // Note: This operator is relevant for the file format, do not modify
  // unless you know exactly what you're doing!
  if (mJunction.has_value() != rhs.mJunction.has_value()) {
    return rhs.mJunction.has_value();
  } else if (mVia.has_value() != rhs.mVia.has_value()) {
    return rhs.mVia.has_value();
  } else if (mPad.has_value() != rhs.mPad.has_value()) {
    return rhs.mPad.has_value();
  } else if (mFootprintPad.has_value() != rhs.mFootprintPad.has_value()) {
    return rhs.mFootprintPad.has_value();
  } else if (mJunction) {
    return (*mJunction) < (*rhs.mJunction);
  } else if (mVia) {
    return (*mVia) < (*rhs.mVia);
  } else if (mPad) {
    return (*mPad) < (*rhs.mPad);
  } else if (mFootprintPad) {
    if (mFootprintPad->device != rhs.mFootprintPad->device) {
      return mFootprintPad->device < rhs.mFootprintPad->device;
    } else {
      return mFootprintPad->pad < rhs.mFootprintPad->pad;
    }
  } else {
    qWarning() << "Unhandled branch in TraceAnchor::operator<().";
    return false;
  }
}

TraceAnchor& TraceAnchor::operator=(const TraceAnchor& rhs) noexcept {
  mJunction = rhs.mJunction;
  mVia = rhs.mVia;
  mPad = rhs.mPad;
  mFootprintPad = rhs.mFootprintPad;
  return *this;
}

TraceAnchor TraceAnchor::junction(const Uuid& junction) noexcept {
  return TraceAnchor(junction, std::nullopt, std::nullopt, std::nullopt);
}

TraceAnchor TraceAnchor::via(const Uuid& via) noexcept {
  return TraceAnchor(std::nullopt, via, std::nullopt, std::nullopt);
}

TraceAnchor TraceAnchor::pad(const Uuid& pad) noexcept {
  return TraceAnchor(std::nullopt, std::nullopt, pad, std::nullopt);
}

TraceAnchor TraceAnchor::footprintPad(const Uuid& device,
                                      const Uuid& pad) noexcept {
  return TraceAnchor(std::nullopt, std::nullopt, std::nullopt,
                     PadAnchor{device, pad});
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Trace::Trace(const Trace& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mLayer(other.mLayer),
    mWidth(other.mWidth),
    mP1(other.mP1),
    mP2(other.mP2) {
}

Trace::Trace(const Uuid& uuid, const Trace& other) noexcept : Trace(other) {
  mUuid = uuid;
}

Trace::Trace(const Uuid& uuid, const Layer& layer, const PositiveLength& width,
             const TraceAnchor& a, const TraceAnchor& b) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayer(&layer),
    mWidth(width),
    mP1(a),
    mP2(b) {
  normalizeAnchors(mP1, mP2);
}

Trace::Trace(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayer(deserialize<const Layer*>(node.getChild("layer/@0"))),
    mWidth(deserialize<PositiveLength>(node.getChild("width/@0"))),
    mP1(node.getChild("from")),
    mP2(node.getChild("to")) {
  normalizeAnchors(mP1, mP2);
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

bool Trace::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
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

bool Trace::setAnchors(TraceAnchor a, TraceAnchor b) noexcept {
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

void Trace::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", *mLayer);
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

bool Trace::operator==(const Trace& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mP1 != rhs.mP1) return false;
  if (mP2 != rhs.mP2) return false;
  return true;
}

Trace& Trace::operator=(const Trace& rhs) noexcept {
  setUuid(rhs.mUuid);
  setLayer(*rhs.mLayer);
  setWidth(rhs.mWidth);
  setAnchors(rhs.mP1, rhs.mP2);
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Trace::normalizeAnchors(TraceAnchor& start, TraceAnchor& end) noexcept {
  if (end < start) {
    std::swap(start, end);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
