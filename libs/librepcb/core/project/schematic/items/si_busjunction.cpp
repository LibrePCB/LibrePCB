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
#include "si_busjunction.h"

#include "../../circuit/bus.h"
#include "si_bussegment.h"
#include "si_netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_BusJunction::SI_BusJunction(SI_BusSegment& segment, const Uuid& uuid,
                               const Point& position)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mSegment(segment),
    mJunction(uuid, position) {
  connect(&mSegment.getBus(), &Bus::nameChanged, this,
          [this]() { onEdited.notify(Event::BusNameChanged); });
}

SI_BusJunction::~SI_BusJunction() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SI_BusJunction::isVisibleJunction() const noexcept {
  return (mRegisteredBusLines.count() > 2);
}

bool SI_BusJunction::isUsed() const noexcept {
  return (!mRegisteredBusLines.isEmpty()) || (!mRegisteredNetLines.isEmpty());
}

bool SI_BusJunction::isOpen() const noexcept {
  return ((mRegisteredBusLines.count() + mRegisteredNetLines.count()) <= 1);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_BusJunction::setPosition(const Point& position) noexcept {
  if (mJunction.setPosition(position)) {
    foreach (SI_BusLine* line, mRegisteredBusLines) {
      line->updatePositions();
    }
    foreach (SI_NetLine* line, mRegisteredNetLines) {
      line->updatePositions();
    }
    onEdited.notify(Event::PositionChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_BusJunction::addToSchematic() {
  if (isAddedToSchematic() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic();
}

void SI_BusJunction::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic();
}

void SI_BusJunction::registerBusLine(SI_BusLine& l) {
  if ((!isAddedToSchematic()) || (mRegisteredBusLines.contains(&l)) ||
      (&l.getBusSegment() != &mSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBusLines.insert(&l);
  if (mRegisteredBusLines.count() <= 3) {
    onEdited.notify(Event::JunctionChanged);
  }
}

void SI_BusJunction::unregisterBusLine(SI_BusLine& l) {
  if ((!isAddedToSchematic()) || (!mRegisteredBusLines.contains(&l))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBusLines.remove(&l);
  if (mRegisteredBusLines.count() <= 2) {
    onEdited.notify(Event::JunctionChanged);
  }
}

void SI_BusJunction::registerNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
  if (mRegisteredNetLines.count() == 1) {
    onEdited.notify(Event::JunctionChanged);
  }
}

void SI_BusJunction::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  if (mRegisteredNetLines.count() == 0) {
    onEdited.notify(Event::JunctionChanged);
  }
}

NetLineAnchor SI_BusJunction::toNetLineAnchor() const noexcept {
  return NetLineAnchor::busJunction(mSegment.getUuid(), mJunction.getUuid());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
