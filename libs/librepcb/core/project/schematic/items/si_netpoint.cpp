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
#include "../../erc/ercmsg.h"
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
    mNetSegment(segment),
    mJunction(uuid, position) {
  // create the graphics item
  mGraphicsItem.reset(new SGI_NetPoint(*this));
  mGraphicsItem->setPos(mJunction.getPosition().toPxQPointF());

  // create ERC messages
  mErcMsgDeadNetPoint.reset(
      new ErcMsg(mSchematic.getProject(), *this, mJunction.getUuid().toStr(),
                 "Dead", ErcMsg::ErcMsgType_t::SchematicError,
                 tr("Dead net point in schematic page \"%1\": %2")
                     .arg(*mSchematic.getName())
                     .arg(mJunction.getUuid().toStr())));
}

SI_NetPoint::~SI_NetPoint() noexcept {
  mGraphicsItem.reset();
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
    mGraphicsItem->setPos(position.toPxQPointF());
    foreach (SI_NetLine* line, mRegisteredNetLines) { line->updateLine(); }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetPoint::addToSchematic() {
  if (isAddedToSchematic() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  mErcMsgDeadNetPoint->setVisible(true);
  SI_Base::addToSchematic(mGraphicsItem.data());
}

void SI_NetPoint::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mHighlightChangedConnection);
  mErcMsgDeadNetPoint->setVisible(false);
  SI_Base::removeFromSchematic(mGraphicsItem.data());
}

void SI_NetPoint::registerNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
  mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

void SI_NetPoint::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
  mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_NetPoint::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape().translated(
      mJunction.getPosition().toPxQPointF());
}

void SI_NetPoint::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
