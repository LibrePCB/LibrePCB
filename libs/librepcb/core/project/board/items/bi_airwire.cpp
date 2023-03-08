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
#include "bi_airwire.h"

#include "../../circuit/netsignal.h"
#include "bi_netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_AirWire::BI_AirWire(Board& board, const NetSignal& netsignal,
                       const BI_NetLineAnchor& p1, const BI_NetLineAnchor& p2)
  : BI_Base(board), mNetSignal(netsignal), mP1(p1), mP2(p2) {
  mGraphicsItem.reset(new BGI_AirWire(*this));
}

BI_AirWire::~BI_AirWire() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BI_AirWire::isVertical() const noexcept {
  return (mP1.getPosition() == mP2.getPosition());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_AirWire::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mHighlightChangedConnection =
      connect(&mNetSignal, &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  BI_Base::addToBoard(mGraphicsItem.data());
}

void BI_AirWire::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mHighlightChangedConnection);
  BI_Base::removeFromBoard(mGraphicsItem.data());
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_AirWire::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape();
}

void BI_AirWire::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

bool BI_AirWire::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
