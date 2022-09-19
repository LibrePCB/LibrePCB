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

CircleGraphicsItem::CircleGraphicsItem(Circle& circle,
                                       const IF_GraphicsLayerProvider& lp,
                                       QGraphicsItem* parent) noexcept
  : PrimitiveCircleGraphicsItem(parent),
    mCircle(circle),
    mLayerProvider(lp),
    mEditedSlot(*this, &CircleGraphicsItem::circleEdited) {
  setPosition(mCircle.getCenter());
  setDiameter(positiveToUnsigned(mCircle.getDiameter()));
  setLineWidth(mCircle.getLineWidth());
  setLineLayer(mLayerProvider.getLayer(*mCircle.getLayerName()));
  updateFillLayer();
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  // register to the circle to get attribute updates
  mCircle.onEdited.attach(mEditedSlot);
}

CircleGraphicsItem::~CircleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CircleGraphicsItem::circleEdited(const Circle& circle,
                                      Circle::Event event) noexcept {
  switch (event) {
    case Circle::Event::LayerNameChanged:
      setLineLayer(mLayerProvider.getLayer(*circle.getLayerName()));
      updateFillLayer();  // required if the area is filled with the line layer
      break;
    case Circle::Event::LineWidthChanged:
      setLineWidth(circle.getLineWidth());
      break;
    case Circle::Event::IsFilledChanged:
    case Circle::Event::IsGrabAreaChanged:
      updateFillLayer();
      break;
    case Circle::Event::CenterChanged:
      setPosition(circle.getCenter());
      break;
    case Circle::Event::DiameterChanged:
      setDiameter(positiveToUnsigned(circle.getDiameter()));
      break;
    default:
      qWarning()
          << "Unhandled switch-case in CircleGraphicsItem::circleEdited():"
          << static_cast<int>(event);
      break;
  }
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
