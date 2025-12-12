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
#include "si_buslabel.h"

#include "../../../utils/scopeguard.h"
#include "../../circuit/bus.h"
#include "../../circuit/circuit.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_bussegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_BusLabel::SI_BusLabel(SI_BusSegment& segment, const NetLabel& label)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mSegment(segment),
    mNetLabel(label),
    mAnchorPosition() {
  updateAnchor();
}

SI_BusLabel::~SI_BusLabel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_BusLabel::setPosition(const Point& position) noexcept {
  if (mNetLabel.setPosition(position)) {
    onEdited.notify(Event::PositionChanged);
    updateAnchor();
  }
}

void SI_BusLabel::setRotation(const Angle& rotation) noexcept {
  if (mNetLabel.setRotation(rotation)) {
    onEdited.notify(Event::RotationChanged);
  }
}

void SI_BusLabel::setMirrored(const bool mirrored) noexcept {
  if (mNetLabel.setMirrored(mirrored)) {
    onEdited.notify(Event::MirroredChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_BusLabel::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNameChangedConnection =
      connect(&mSegment.getBus(), &Bus::nameChanged,
              [this]() { onEdited.notify(Event::BusNameChanged); });
  SI_Base::addToSchematic();
}

void SI_BusLabel::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mNameChangedConnection);
  SI_Base::removeFromSchematic();
}

void SI_BusLabel::updateAnchor() noexcept {
  const Point p = mSegment.calcNearestPoint(mNetLabel.getPosition());
  if (p != mAnchorPosition) {
    mAnchorPosition = p;
    onEdited.notify(Event::AnchorPositionChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
