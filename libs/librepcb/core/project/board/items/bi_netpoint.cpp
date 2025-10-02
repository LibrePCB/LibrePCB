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
#include "bi_netpoint.h"

#include "../../circuit/netsignal.h"
#include "../board.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const Uuid& uuid,
                         const Point& position)
  : BI_Base(segment.getBoard()),
    onEdited(*this),
    mNetSegment(segment),
    mJunction(uuid, position),
    mLayerOfTraces(nullptr),
    mMaxTraceWidth(std::nullopt),
    mOnNetLineEditedSlot(*this, &BI_NetPoint::netLineEdited) {
}

BI_NetPoint::~BI_NetPoint() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

TraceAnchor BI_NetPoint::toTraceAnchor() const noexcept {
  return TraceAnchor::junction(mJunction.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetPoint::setPosition(const Point& position) noexcept {
  if (mJunction.setPosition(position)) {
    foreach (BI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
      mBoard.invalidatePlanes(&netLine->getLayer());
    }
    onEdited.notify(Event::PositionChanged);
    if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
      mBoard.scheduleAirWiresRebuild(netsignal);
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_NetPoint::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently already added to the board.");
  } else if (isUsed()) {
    throw LogicError(__FILE__, __LINE__, "NetPoint is currently in use.");
  }
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  BI_Base::addToBoard();

  if (const NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mNetSignalNameChangedConnection =
        connect(netsignal, &NetSignal::nameChanged, this,
                [this]() { onEdited.notify(Event::NetSignalNameChanged); });
  }
}

void BI_NetPoint::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently not added to the board.");
  } else if (isUsed()) {
    throw LogicError(__FILE__, __LINE__, "NetPoint is currently in use.");
  }
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  BI_Base::removeFromBoard();

  if (mNetSignalNameChangedConnection) {
    disconnect(mNetSignalNameChangedConnection);
    mNetSignalNameChangedConnection = QMetaObject::Connection();
  }
}

void BI_NetPoint::registerNetLine(BI_NetLine& netline) {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently not added to the board.");
  } else if (mRegisteredNetLines.contains(&netline)) {
    throw LogicError(__FILE__, __LINE__,
                     "NetLine is already registered to the NetPoint.");
  } else if (&netline.getNetSegment() != &mNetSegment) {
    throw LogicError(__FILE__, __LINE__,
                     "NetLine has different NetSegment than the NetPoint.");
  } else if ((mRegisteredNetLines.count() > 0) &&
             (&netline.getLayer() != getLayerOfTraces())) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint already has NetLines on different layer.");
  }
  mRegisteredNetLines.insert(&netline);
  netline.onEdited.attach(mOnNetLineEditedSlot);
  updateLayerOfTraces();
  updateMaxTraceWidth();
}

void BI_NetPoint::unregisterNetLine(BI_NetLine& netline) {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__, "NetLine is not part of a board.");
  } else if ((!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__, "NetLine is not registered.");
  }
  mRegisteredNetLines.remove(&netline);
  netline.onEdited.detach(mOnNetLineEditedSlot);
  updateLayerOfTraces();
  updateMaxTraceWidth();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_NetPoint::netLineEdited(const BI_NetLine& obj,
                                BI_NetLine::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_NetLine::Event::WidthChanged:
      updateMaxTraceWidth();
      break;
    default:
      break;
  }
}

void BI_NetPoint::updateLayerOfTraces() noexcept {
  const Layer* layer = nullptr;
  if (!mRegisteredNetLines.isEmpty()) {
    layer = &(*mRegisteredNetLines.begin())->getLayer();
  }
  if (layer != mLayerOfTraces) {
    mLayerOfTraces = layer;
    onEdited.notify(Event::LayerOfTracesChanged);
  }
}

void BI_NetPoint::updateMaxTraceWidth() noexcept {
  const std::optional<PositiveLength> width = getMaxLineWidth();
  if (width != mMaxTraceWidth) {
    mMaxTraceWidth = width;
    onEdited.notify(Event::MaxTraceWidthChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
