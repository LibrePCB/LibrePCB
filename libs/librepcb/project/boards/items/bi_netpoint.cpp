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
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const BI_NetPoint& other)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mUuid(Uuid::createRandom()),
    mPosition(other.mPosition) {
  init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const SExpression& node)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mPosition(node.getChildByPath("position")) {
  init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const Point& position)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mUuid(Uuid::createRandom()),
    mPosition(position) {
  init();
}

void BI_NetPoint::init() {
  // create the graphics item
  mGraphicsItem.reset(new BGI_NetPoint(*this));
  mGraphicsItem->setPos(mPosition.toPxQPointF());

  // create ERC messages
  mErcMsgDeadNetPoint.reset(
      new ErcMsg(mBoard.getProject(), *this, mUuid.toStr(), "Dead",
                 ErcMsg::ErcMsgType_t::BoardError,
                 QString(tr("Dead net point in board \"%1\": %2"))
                     .arg(*mBoard.getName())
                     .arg(mUuid.toStr())));
}

BI_NetPoint::~BI_NetPoint() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

NetSignal& BI_NetPoint::getNetSignalOfNetSegment() const noexcept {
  return mNetSegment.getNetSignal();
}

GraphicsLayer* BI_NetPoint::getLayerOfLines() const noexcept {
  auto it = mRegisteredNetLines.constBegin();
  return (it != mRegisteredNetLines.constEnd()) ? &((*it)->getLayer())
                                                : nullptr;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetPoint::setPosition(const Point& position) noexcept {
  if (position != mPosition) {
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    foreach (BI_NetLine* line, mRegisteredNetLines) { line->updateLine(); }
    mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
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
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently in use.");
  }
  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  mErcMsgDeadNetPoint->setVisible(true);
  BI_Base::addToBoard(mGraphicsItem.data());
  mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_NetPoint::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently not added to the board.");
  } else if (isUsed()) {
    throw LogicError(__FILE__, __LINE__,
                     "NetPoint is currently in use.");
  }
  disconnect(mHighlightChangedConnection);
  mErcMsgDeadNetPoint->setVisible(false);
  BI_Base::removeFromBoard(mGraphicsItem.data());
  mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_NetPoint::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment) ||
      ((mRegisteredNetLines.count() > 0) &&
       (&netline.getLayer() != getLayerOfLines()))) {
    throw LogicError(__FILE__, __LINE__);
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

void BI_NetPoint::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_NetPoint::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape().translated(mPosition.toPxQPointF());
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

}  // namespace project
}  // namespace librepcb
