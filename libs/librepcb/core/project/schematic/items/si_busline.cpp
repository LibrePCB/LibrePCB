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
#include "si_busline.h"

#include "../../../utils/scopeguard.h"
#include "../../circuit/bus.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_busjunction.h"
#include "si_bussegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_BusLine::SI_BusLine(SI_BusSegment& segment, const Uuid& uuid,
                       SI_BusJunction& a, SI_BusJunction& b,
                       const UnsignedLength& width)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mSegment(segment),
    mNetLine(uuid, width, NetLineAnchor::junction(a.getUuid()),
             NetLineAnchor::junction(b.getUuid())),
    mP1(&a),
    mP2(&b) {
  // Sort anchors to get a canonical file format.
  if (mP1->getUuid() != *mNetLine.getP1().tryGetJunction()) {
    std::swap(mP1, mP2);
  }

  // check if both netpoints are different
  if (mP1 == mP2) {
    throw LogicError(__FILE__, __LINE__,
                     "SI_BusLine: both endpoints are the same.");
  }

  connect(&mSegment.getBus(), &Bus::nameChanged, this,
          [this]() { onEdited.notify(Event::BusNameChanged); });
}

SI_BusLine::~SI_BusLine() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

SI_BusJunction* SI_BusLine::getOtherPoint(
    const SI_BusJunction& firstPoint) const noexcept {
  if (&firstPoint == mP1) {
    return mP2;
  } else if (&firstPoint == mP2) {
    return mP1;
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_BusLine::setWidth(const UnsignedLength& width) noexcept {
  mNetLine.setWidth(width);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_BusLine::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP1->registerBusLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP1->unregisterBusLine(*this); });
  mP2->registerBusLine(*this);  // can throw

  SI_Base::addToSchematic();
  sg.dismiss();
}

void SI_BusLine::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP2->unregisterBusLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP2->registerBusLine(*this); });
  mP1->unregisterBusLine(*this);  // can throw

  SI_Base::removeFromSchematic();
  sg.dismiss();
}

void SI_BusLine::updatePositions() noexcept {
  onEdited.notify(Event::PositionsChanged);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
