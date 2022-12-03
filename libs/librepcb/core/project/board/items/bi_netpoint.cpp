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
#include "../../erc/ercmsg.h"
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
    mNetSegment(segment),
    mJunction(uuid, position) {
  // create the graphics item
  mGraphicsItem.reset(new BGI_NetPoint(*this));
  mGraphicsItem->setPos(mJunction.getPosition().toPxQPointF());

  // create ERC messages
  mErcMsgDeadNetPoint.reset(new ErcMsg(mBoard.getProject(), *this,
                                       mJunction.getUuid().toStr(), "Dead",
                                       ErcMsg::ErcMsgType_t::BoardError,
                                       tr("Dead net point in board \"%1\": %2")
                                           .arg(*mBoard.getName())
                                           .arg(mJunction.getUuid().toStr())));
}

BI_NetPoint::~BI_NetPoint() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

GraphicsLayer* BI_NetPoint::getLayerOfLines() const noexcept {
  auto it = mRegisteredNetLines.constBegin();
  return (it != mRegisteredNetLines.constEnd()) ? &((*it)->getLayer())
                                                : nullptr;
}

TraceAnchor BI_NetPoint::toTraceAnchor() const noexcept {
  return TraceAnchor::junction(mJunction.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetPoint::setPosition(const Point& position) noexcept {
  if (mJunction.setPosition(position)) {
    mGraphicsItem->setPos(position.toPxQPointF());
    foreach (BI_NetLine* line, mRegisteredNetLines) { line->updateLine(); }
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
    mHighlightChangedConnection =
        connect(netsignal, &NetSignal::highlightedChanged,
                [this]() { mGraphicsItem->update(); });
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  mErcMsgDeadNetPoint->setVisible(true);
  BI_Base::addToBoard(mGraphicsItem.data());
}

void BI_NetPoint::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently not added to the board.");
  } else if (isUsed()) {
    throw LogicError(__FILE__, __LINE__, "NetPoint is currently in use.");
  }
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    disconnect(mHighlightChangedConnection);
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  mErcMsgDeadNetPoint->setVisible(false);
  BI_Base::removeFromBoard(mGraphicsItem.data());
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
             (&netline.getLayer() != getLayerOfLines())) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint already has NetLines on different layer.");
  }
  mRegisteredNetLines.insert(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
  mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

void BI_NetPoint::unregisterNetLine(BI_NetLine& netline) {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__, "NetLine is not part of a board.");
  } else if ((!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__, "NetLine is not registered.");
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
  mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_NetPoint::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape().translated(
      mJunction.getPosition().toPxQPointF());
}

bool BI_NetPoint::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_NetPoint::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
