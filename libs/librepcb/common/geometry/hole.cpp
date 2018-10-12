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
#include "hole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Hole::Hole(const Hole& other) noexcept
  : mUuid(other.mUuid), mPosition(other.mPosition), mDiameter(other.mDiameter) {
}

Hole::Hole(const Uuid& uuid, const Hole& other) noexcept : Hole(other) {
  mUuid = uuid;
}

Hole::Hole(const Uuid& uuid, const Point& position,
           const PositiveLength& diameter) noexcept
  : mUuid(uuid), mPosition(position), mDiameter(diameter) {
}

Hole::Hole(const SExpression& node)
  : mUuid(Uuid::createRandom()),  // backward compatibility, remove this some
                                  // time!
    mPosition(0, 0),
    mDiameter(1) {
  if (node.tryGetChildByPath("position")) {
    mPosition = Point(node.getChildByPath("position"));
  } else {
    // backward compatibility, remove this some time!
    mPosition = Point(node.getChildByPath("pos"));
  }
  if (node.tryGetChildByPath("diameter")) {
    mDiameter = node.getValueByPath<PositiveLength>("diameter");
  } else {
    // backward compatibility, remove this some time!
    mDiameter = node.getValueByPath<PositiveLength>("dia");
  }
  if (node.getChildByIndex(0).isString()) {
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
  }
}

Hole::~Hole() noexcept {
  Q_ASSERT(mObservers.isEmpty());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Hole::setPosition(const Point& position) noexcept {
  if (position == mPosition) return;
  mPosition = position;
  foreach (IF_HoleObserver* object, mObservers) {
    object->holePositionChanged(mPosition);
  }
}

void Hole::setDiameter(const PositiveLength& diameter) noexcept {
  if (diameter == mDiameter) return;
  mDiameter = diameter;
  foreach (IF_HoleObserver* object, mObservers) {
    object->holeDiameterChanged(mDiameter);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Hole::registerObserver(IF_HoleObserver& object) const noexcept {
  mObservers.insert(&object);
}

void Hole::unregisterObserver(IF_HoleObserver& object) const noexcept {
  mObservers.remove(&object);
}

void Hole::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("diameter", mDiameter, false);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Hole::operator==(const Hole& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mDiameter != rhs.mDiameter) return false;
  return true;
}

Hole& Hole::operator=(const Hole& rhs) noexcept {
  mUuid     = rhs.mUuid;
  mPosition = rhs.mPosition;
  mDiameter = rhs.mDiameter;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
