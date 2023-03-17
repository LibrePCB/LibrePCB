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
                       SI_NetLineAnchor& startPoint, SI_NetLineAnchor& endPoint,
                       const UnsignedLength& width)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mNetSegment(segment),
    mNetLine(uuid, width, startPoint.toNetLineAnchor(),
             endPoint.toNetLineAnchor()),
    mStartPoint(&startPoint),
    mEndPoint(&endPoint) {
  // check if both netpoints are different
  if (mStartPoint == mEndPoint) {
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
  if (&firstPoint == mStartPoint) {
    return mEndPoint;
  } else if (&firstPoint == mEndPoint) {
    return mStartPoint;
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

  mStartPoint->registerNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mStartPoint->unregisterNetLine(*this); });
  mEndPoint->registerNetLine(*this);  // can throw

  SI_Base::addToSchematic();
  sg.dismiss();
}

void SI_NetLine::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mEndPoint->unregisterNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mEndPoint->registerNetLine(*this); });
  mStartPoint->unregisterNetLine(*this);  // can throw

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
