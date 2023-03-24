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
#include "si_netpoint.h"

#include "../../circuit/netsignal.h"
#include "si_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetPoint::SI_NetPoint(SI_NetSegment& segment, const Uuid& uuid,
                         const Point& position)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mNetSegment(segment),
    mJunction(uuid, position) {
  connect(&mNetSegment.getNetSignal(), &NetSignal::nameChanged, this,
          [this]() { onEdited.notify(Event::NetSignalNameChanged); });
}

SI_NetPoint::~SI_NetPoint() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SI_NetPoint::isVisibleJunction() const noexcept {
  return (mRegisteredNetLines.count() > 2);
}

bool SI_NetPoint::isOpenLineEnd() const noexcept {
  return (mRegisteredNetLines.count() <= 1);
}

NetSignal& SI_NetPoint::getNetSignalOfNetSegment() const noexcept {
  return mNetSegment.getNetSignal();
}

NetLineAnchor SI_NetPoint::toNetLineAnchor() const noexcept {
  return NetLineAnchor::junction(mJunction.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetPoint::setPosition(const Point& position) noexcept {
  if (mJunction.setPosition(position)) {
    foreach (SI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
    onEdited.notify(Event::PositionChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetPoint::addToSchematic() {
  if (isAddedToSchematic() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic();
}

void SI_NetPoint::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic();
}

void SI_NetPoint::registerNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
  if (mRegisteredNetLines.count() <= 3) {
    onEdited.notify(Event::JunctionChanged);
  }
}

void SI_NetPoint::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  if (mRegisteredNetLines.count() <= 2) {
    onEdited.notify(Event::JunctionChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
