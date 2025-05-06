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

#include "graphicslayer.h"
#include "graphicslayerlist.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CircleGraphicsItem::CircleGraphicsItem(Circle& circle,
                                       const GraphicsLayerList& layers,
                                       QGraphicsItem* parent) noexcept
  : PrimitiveCircleGraphicsItem(parent),
    mCircle(circle),
    mLayers(layers),
    mEditedSlot(*this, &CircleGraphicsItem::circleEdited) {
  setPosition(mCircle.getCenter());
  setDiameter(positiveToUnsigned(mCircle.getDiameter()));
  setLineWidth(mCircle.getLineWidth());
  setLineLayer(mLayers.get(mCircle.getLayer()));
  updateFillLayer();
  updateZValue();
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
    case Circle::Event::LayerChanged:
      setLineLayer(mLayers.get(circle.getLayer()));
      updateFillLayer();  // required if the area is filled with the line layer
      break;
    case Circle::Event::LineWidthChanged:
      setLineWidth(circle.getLineWidth());
      break;
    case Circle::Event::IsFilledChanged:
    case Circle::Event::IsGrabAreaChanged:
      updateFillLayer();
      updateZValue();
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
    setFillLayer(mLayers.get(mCircle.getLayer()));
  } else if (mCircle.isGrabArea()) {
    setFillLayer(mLayers.grabArea(mCircle.getLayer()));
  } else {
    setFillLayer(nullptr);
  }
}

void CircleGraphicsItem::updateZValue() noexcept {
  // Fix for https://github.com/LibrePCB/LibrePCB/issues/1278.
  if (mCircle.isFilled()) {
    setZValue(0);
  } else if (mCircle.isGrabArea()) {
    setZValue(-1);
  } else {
    setZValue(1);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
