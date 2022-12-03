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

#include "../../circuit/netsignal.h"
#include "../boardlayerstack.h"
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
  : BI_Base(netsegment.getBoard()), mVia(via), mNetSegment(netsegment) {
  init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const SExpression& node,
               const Version& fileFormat)
  : BI_Base(netsegment.getBoard()),
    mVia(node, fileFormat),
    mNetSegment(netsegment) {
  init();
}

void BI_Via::init() {
  // create the graphics item
  mGraphicsItem.reset(new BGI_Via(*this));
  mGraphicsItem->setPos(mVia.getPosition().toPxQPointF());

  // connect to the "attributes changed" signal of the board
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_Via::boardOrNetAttributesChanged);
}

BI_Via::~BI_Via() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BI_Via::isOnLayer(const QString& layerName) const noexcept {
  return GraphicsLayer::isCopperLayer(layerName);
}

TraceAnchor BI_Via::toTraceAnchor() const noexcept {
  return TraceAnchor::via(mVia.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Via::setPosition(const Point& position) noexcept {
  if (mVia.setPosition(position)) {
    mGraphicsItem->setPos(position.toPxQPointF());
    foreach (BI_NetLine* netline, mRegisteredNetLines) {
      netline->updateLine();
    }
    if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
      mBoard.scheduleAirWiresRebuild(netsignal);
    }
  }
}

void BI_Via::setShape(Via::Shape shape) noexcept {
  if (mVia.setShape(shape)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Via::setSize(const PositiveLength& size) noexcept {
  if (mVia.setSize(size)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Via::setDrillDiameter(const PositiveLength& diameter) noexcept {
  if (mVia.setDrillDiameter(diameter)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Via::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mConnections.append(connect(netsignal, &NetSignal::nameChanged, this,
                                &BI_Via::boardOrNetAttributesChanged));
    mConnections.append(connect(netsignal, &NetSignal::highlightedChanged,
                                [this]() { mGraphicsItem->update(); }));
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  BI_Base::addToBoard(mGraphicsItem.data());
  mGraphicsItem->updateCacheAndRepaint();  // Force updating tooltip.
}

void BI_Via::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  while (!mConnections.isEmpty()) {
    disconnect(mConnections.takeLast());
  }
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  BI_Base::removeFromBoard(mGraphicsItem.data());
}

void BI_Via::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::serialize(SExpression& root) const {
  mVia.serialize(root);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_Via::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape().translated(mVia.getPosition().toPxQPointF());
}

bool BI_Via::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_Via::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Via::boardOrNetAttributesChanged() {
  mGraphicsItem->updateCacheAndRepaint();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
