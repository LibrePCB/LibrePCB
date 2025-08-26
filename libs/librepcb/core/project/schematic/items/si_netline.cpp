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
#include "si_netline.h"

#include "../../../utils/scopeguard.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_netpoint.h"
#include "si_netsegment.h"
#include "si_symbol.h"
#include "si_symbolpin.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetLine::SI_NetLine(SI_NetSegment& segment, const Uuid& uuid,
                       SI_NetLineAnchor& a, SI_NetLineAnchor& b,
                       const UnsignedLength& width)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mNetSegment(segment),
    mNetLine(uuid, width, a.toNetLineAnchor(), b.toNetLineAnchor()),
    mP1(&a),
    mP2(&b) {
  // Sort anchors to get a canonical file format.
  if (mP2->toNetLineAnchor() < mP1->toNetLineAnchor()) {
    std::swap(mP1, mP2);
  }

  // check if both netpoints are different
  if (mP1 == mP2) {
    throw LogicError(__FILE__, __LINE__,
                     "SI_NetLine: both endpoints are the same.");
  }

  connect(&mNetSegment.getNetSignal(), &NetSignal::nameChanged, this,
          [this]() { onEdited.notify(Event::NetSignalNameChanged); });
}

SI_NetLine::~SI_NetLine() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

SI_NetLineAnchor* SI_NetLine::getOtherPoint(
    const SI_NetLineAnchor& firstPoint) const noexcept {
  if (&firstPoint == mP1) {
    return mP2;
  } else if (&firstPoint == mP2) {
    return mP1;
  } else {
    return nullptr;
  }
}

NetSignal& SI_NetLine::getNetSignalOfNetSegment() const noexcept {
  return getNetSegment().getNetSignal();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetLine::setWidth(const UnsignedLength& width) noexcept {
  mNetLine.setWidth(width);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetLine::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP1->registerNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP1->unregisterNetLine(*this); });
  mP2->registerNetLine(*this);  // can throw

  SI_Base::addToSchematic();
  sg.dismiss();
}

void SI_NetLine::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP2->unregisterNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP2->registerNetLine(*this); });
  mP1->unregisterNetLine(*this);  // can throw

  SI_Base::removeFromSchematic();
  sg.dismiss();
}

void SI_NetLine::updatePositions() noexcept {
  onEdited.notify(Event::PositionsChanged);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
