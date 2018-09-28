/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "holegraphicsitem.h"

#include "../graphics/graphicslayer.h"
#include "origincrossgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

HoleGraphicsItem::HoleGraphicsItem(Hole&                           hole,
                                   const IF_GraphicsLayerProvider& lp,
                                   QGraphicsItem* parent) noexcept
  : PrimitiveCircleGraphicsItem(parent), mHole(hole), mLayerProvider(lp) {
  setPosition(mHole.getPosition());
  setDiameter(positiveToUnsigned(mHole.getDiameter()));
  setLineLayer(mLayerProvider.getLayer(GraphicsLayer::sBoardDrillsNpth));
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  // add origin cross
  mOriginCrossGraphicsItem.reset(new OriginCrossGraphicsItem(this));
  mOriginCrossGraphicsItem->setRotation(Angle::deg45());
  mOriginCrossGraphicsItem->setSize(positiveToUnsigned(mHole.getDiameter()) +
                                    UnsignedLength(500000));
  mOriginCrossGraphicsItem->setLayer(
      mLayerProvider.getLayer(GraphicsLayer::sTopReferences));

  // register to the text to get attribute updates
  mHole.registerObserver(*this);
}

HoleGraphicsItem::~HoleGraphicsItem() noexcept {
  mHole.unregisterObserver(*this);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath HoleGraphicsItem::shape() const noexcept {
  return PrimitiveCircleGraphicsItem::shape() +
         mOriginCrossGraphicsItem->shape();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HoleGraphicsItem::holePositionChanged(const Point& newPos) noexcept {
  setPosition(newPos);
}

void HoleGraphicsItem::holeDiameterChanged(
    const PositiveLength& newDiameter) noexcept {
  setDiameter(positiveToUnsigned(newDiameter));
  mOriginCrossGraphicsItem->setSize(positiveToUnsigned(mHole.getDiameter()) +
                                    UnsignedLength(500000));
}

QVariant HoleGraphicsItem::itemChange(GraphicsItemChange change,
                                      const QVariant&    value) noexcept {
  if (change == ItemSelectedChange && mOriginCrossGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
