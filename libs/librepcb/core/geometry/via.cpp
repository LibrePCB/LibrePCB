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
#include "via.h"

#include "../types/layer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Via::Via(const Via& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mStartLayer(other.mStartLayer),
    mEndLayer(other.mEndLayer),
    mPosition(other.mPosition),
    mSize(other.mSize),
    mDrillDiameter(other.mDrillDiameter) {
}

Via::Via(const Uuid& uuid, const Via& other) noexcept : Via(other) {
  mUuid = uuid;
}

Via::Via(const Uuid& uuid, const Layer& startLayer, const Layer& endLayer,
         const Point& position, const PositiveLength& size,
         const PositiveLength& drillDiameter) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mStartLayer(&startLayer),
    mEndLayer(&endLayer),
    mPosition(position),
    mSize(size),
    mDrillDiameter(drillDiameter) {
}

Via::Via(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mStartLayer(&deserialize<const Layer&>(node.getChild("from/@0"))),
    mEndLayer(&deserialize<const Layer&>(node.getChild("to/@0"))),
    mPosition(node.getChild("position")),
    mSize(deserialize<PositiveLength>(node.getChild("size/@0"))),
    mDrillDiameter(deserialize<PositiveLength>(node.getChild("drill/@0"))) {
  if ((!mStartLayer->isCopper()) || (!mEndLayer->isCopper()) ||
      (mStartLayer->getCopperNumber() >= mEndLayer->getCopperNumber())) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid via layer specification.");
  }
}

Via::~Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Path Via::getOutline(const Length& expansion) const noexcept {
  return getOutline(mSize, expansion);
}

Path Via::getSceneOutline(const Length& expansion) const noexcept {
  return getOutline(expansion).translated(mPosition);
}

bool Via::isThrough() const noexcept {
  return ((mStartLayer == &Layer::topCopper()) &&
          (mEndLayer == &Layer::botCopper()));
}

bool Via::isBlind() const noexcept {
  return (((mStartLayer == &Layer::topCopper()) &&
           (mEndLayer != &Layer::botCopper())) ||
          ((mStartLayer != &Layer::topCopper()) &&
           (mEndLayer == &Layer::botCopper())));
}

bool Via::isBuried() const noexcept {
  return ((mStartLayer != &Layer::topCopper()) &&
          (mEndLayer != &Layer::botCopper()));
}

bool Via::isOnLayer(const Layer& layer) const noexcept {
  return layer.isCopper() && isOnLayer(layer, *mStartLayer, *mEndLayer);
}

QPainterPath Via::toQPainterPathPx(const Length& expansion) const noexcept {
  return toQPainterPathPx(mSize, mDrillDiameter, expansion);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Via::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool Via::setLayers(const Layer& from, const Layer& to) {
  if ((from == *mStartLayer) && (to == *mEndLayer)) {
    return false;
  }

  if ((!from.isCopper()) || (!to.isCopper())) {
    throw LogicError(__FILE__, __LINE__, "Via layer is not a copper layer.");
  }

  if (from.getCopperNumber() >= to.getCopperNumber()) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid via layer specification.");
  }

  mStartLayer = &from;
  mEndLayer = &to;
  onEdited.notify(Event::LayersChanged);
  return true;
}

bool Via::setPosition(const Point& position) noexcept {
  if (position == mPosition) {
    return false;
  }

  mPosition = position;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool Via::setSize(const PositiveLength& size) noexcept {
  if (size == mSize) {
    return false;
  }

  mSize = size;
  onEdited.notify(Event::SizeChanged);
  return true;
}

bool Via::setDrillDiameter(const PositiveLength& diameter) noexcept {
  if (diameter == mDrillDiameter) {
    return false;
  }

  mDrillDiameter = diameter;
  onEdited.notify(Event::DrillDiameterChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Via::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("from", *mStartLayer);
  root.appendChild("to", *mEndLayer);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("size", mSize);
  root.appendChild("drill", mDrillDiameter);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Via::operator==(const Via& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mStartLayer != rhs.mStartLayer) return false;
  if (mEndLayer != rhs.mEndLayer) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mSize != rhs.mSize) return false;
  if (mDrillDiameter != rhs.mDrillDiameter) return false;
  return true;
}

Via& Via::operator=(const Via& rhs) noexcept {
  setUuid(rhs.mUuid);
  setLayers(rhs.getStartLayer(), rhs.getEndLayer());
  setPosition(rhs.mPosition);
  setSize(rhs.mSize);
  setDrillDiameter(rhs.mDrillDiameter);
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Path Via::getOutline(const PositiveLength& size,
                     const Length& expansion) noexcept {
  Length expandedSize = size + (expansion * 2);
  if (expandedSize > 0) {
    return Path::circle(PositiveLength(expandedSize));
  }
  return Path();
}

bool Via::isOnLayer(const Layer& layer, const Layer& from,
                    const Layer& to) noexcept {
  const int nbr = layer.getCopperNumber();
  return ((nbr >= from.getCopperNumber()) && (nbr <= to.getCopperNumber()));
}

QPainterPath Via::toQPainterPathPx(const PositiveLength& size,
                                   const PositiveLength& drillDiameter,
                                   const Length& expansion) noexcept {
  // Avoid creating inverted graphics if drill>size, since it looks correct.
  PositiveLength drill = std::min(drillDiameter, size);

  QPainterPath p = getOutline(size, expansion).toQPainterPathPx();
  p.setFillRule(Qt::OddEvenFill);  // Important to subtract the hole!
  p.addEllipse(QPointF(0, 0), drill->toPx() / 2, drill->toPx() / 2);
  return p;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
