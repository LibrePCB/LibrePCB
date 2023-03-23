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
#include "bi_via.h"

#include "../../../types/layer.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Via::BI_Via(BI_NetSegment& netsegment, const Via& via)
  : BI_Base(netsegment.getBoard()),
    onEdited(*this),
    mVia(via),
    mNetSegment(netsegment),
    mStopMaskOffset() {
  updateStopMaskOffset();

  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Via::updateStopMaskOffset);
}

BI_Via::~BI_Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BI_Via::isOnLayer(const Layer& layer) const noexcept {
  return layer.isCopper();
}

TraceAnchor BI_Via::toTraceAnchor() const noexcept {
  return TraceAnchor::via(mVia.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Via::setPosition(const Point& position) noexcept {
  if (mVia.setPosition(position)) {
    foreach (BI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
    if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
      mBoard.scheduleAirWiresRebuild(netsignal);
    }
    onEdited.notify(Event::PositionChanged);
  }
}

void BI_Via::setSize(const PositiveLength& size) noexcept {
  if (mVia.setSize(size)) {
    onEdited.notify(Event::SizeChanged);
    updateStopMaskOffset();
  }
}

void BI_Via::setDrillDiameter(const PositiveLength& diameter) noexcept {
  if (mVia.setDrillDiameter(diameter)) {
    onEdited.notify(Event::DrillDiameterChanged);
    updateStopMaskOffset();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Via::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mNetSignalNameChangedConnection =
        connect(netsignal, &NetSignal::nameChanged, this,
                [this]() { onEdited.notify(Event::NetSignalNameChanged); });
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
}

void BI_Via::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  if (mNetSignalNameChangedConnection) {
    disconnect(mNetSignalNameChangedConnection);
    mNetSignalNameChangedConnection = QMetaObject::Connection();
  }
}

void BI_Via::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
}

void BI_Via::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
}

void BI_Via::updateStopMaskOffset() noexcept {
  tl::optional<Length> offset;
  if (mBoard.getDesignRules().doesViaRequireStopMaskOpening(
          *mVia.getDrillDiameter())) {
    offset = *mBoard.getDesignRules().getStopMaskClearance().calcValue(
        *mVia.getSize());
  }

  if (offset != mStopMaskOffset) {
    mStopMaskOffset = offset;
    onEdited.notify(Event::StopMaskOffsetChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
