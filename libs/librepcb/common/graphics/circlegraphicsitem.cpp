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
#include "circlegraphicsitem.h"

#include "../graphics/graphicslayer.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CircleGraphicsItem::CircleGraphicsItem(Circle&                         circle,
                                       const IF_GraphicsLayerProvider& lp,
                                       QGraphicsItem* parent) noexcept
  : PrimitiveCircleGraphicsItem(parent), mCircle(circle), mLayerProvider(lp) {
  setPosition(mCircle.getCenter());
  setDiameter(positiveToUnsigned(mCircle.getDiameter()));
  setLineWidth(mCircle.getLineWidth());
  setLineLayer(mLayerProvider.getLayer(*mCircle.getLayerName()));
  updateFillLayer();
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  // register to the circle to get attribute updates
  mCircle.registerObserver(*this);
}

CircleGraphicsItem::~CircleGraphicsItem() noexcept {
  mCircle.unregisterObserver(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CircleGraphicsItem::circleLayerNameChanged(
    const GraphicsLayerName& newLayerName) noexcept {
  setLineLayer(mLayerProvider.getLayer(*newLayerName));
  updateFillLayer();  // required if the area is filled with the line layer
}

void CircleGraphicsItem::circleLineWidthChanged(
    const UnsignedLength& newLineWidth) noexcept {
  setLineWidth(newLineWidth);
}

void CircleGraphicsItem::circleIsFilledChanged(bool newIsFilled) noexcept {
  Q_UNUSED(newIsFilled);
  updateFillLayer();
}

void CircleGraphicsItem::circleIsGrabAreaChanged(bool newIsGrabArea) noexcept {
  Q_UNUSED(newIsGrabArea);
  updateFillLayer();
}

void CircleGraphicsItem::circleCenterChanged(const Point& newCenter) noexcept {
  setPosition(newCenter);
}

void CircleGraphicsItem::circleDiameterChanged(
    const PositiveLength& newDiameter) noexcept {
  setDiameter(positiveToUnsigned(newDiameter));
}

void CircleGraphicsItem::updateFillLayer() noexcept {
  if (mCircle.isFilled()) {
    setFillLayer(mLayerProvider.getLayer(*mCircle.getLayerName()));
  } else if (mCircle.isGrabArea()) {
    setFillLayer(mLayerProvider.getGrabAreaLayer(*mCircle.getLayerName()));
  } else {
    setFillLayer(nullptr);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
