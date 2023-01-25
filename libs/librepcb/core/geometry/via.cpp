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
    mPosition(other.mPosition),
    mSize(other.mSize),
    mDrillDiameter(other.mDrillDiameter) {
}

Via::Via(const Uuid& uuid, const Via& other) noexcept : Via(other) {
  mUuid = uuid;
}

Via::Via(const Uuid& uuid, const Point& position, const PositiveLength& size,
         const PositiveLength& drillDiameter) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mPosition(position),
    mSize(size),
    mDrillDiameter(drillDiameter) {
}

Via::Via(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mPosition(node.getChild("position")),
    mSize(deserialize<PositiveLength>(node.getChild("size/@0"))),
    mDrillDiameter(deserialize<PositiveLength>(node.getChild("drill/@0"))) {
}

Via::~Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Path Via::getOutline(const Length& expansion) const noexcept {
  Length size = mSize + (expansion * 2);
  if (size > 0) {
    return Path::circle(PositiveLength(size));
  }
  return Path();
}

Path Via::getSceneOutline(const Length& expansion) const noexcept {
  return getOutline(expansion).translated(mPosition);
}

QPainterPath Via::toQPainterPathPx(const Length& expansion) const noexcept {
  // Avoid creating inverted graphics if drill>size, since it looks correct.
  PositiveLength drill = std::min(mDrillDiameter, mSize);

  QPainterPath p = getOutline(expansion).toQPainterPathPx();
  p.setFillRule(Qt::OddEvenFill);  // Important to subtract the hole!
  p.addEllipse(QPointF(0, 0), drill->toPx() / 2, drill->toPx() / 2);
  return p;
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
  if (mPosition != rhs.mPosition) return false;
  if (mSize != rhs.mSize) return false;
  if (mDrillDiameter != rhs.mDrillDiameter) return false;
  return true;
}

Via& Via::operator=(const Via& rhs) noexcept {
  setUuid(rhs.mUuid);
  setPosition(rhs.mPosition);
  setSize(rhs.mSize);
  setDrillDiameter(rhs.mDrillDiameter);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
