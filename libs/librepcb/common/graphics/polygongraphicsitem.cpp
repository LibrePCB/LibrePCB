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
#include "polygongraphicsitem.h"

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

PolygonGraphicsItem::PolygonGraphicsItem(Polygon& polygon,
                                         const IF_GraphicsLayerProvider& lp,
                                         QGraphicsItem* parent) noexcept
  : PrimitivePathGraphicsItem(parent),
    mPolygon(polygon),
    mLayerProvider(lp),
    mOnEditedSlot(*this, &PolygonGraphicsItem::polygonEdited) {
  setPath(mPolygon.getPath().toQPainterPathPx());
  setLineWidth(mPolygon.getLineWidth());
  setLineLayer(mLayerProvider.getLayer(*mPolygon.getLayerName()));
  updateFillLayer();
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  // register to the polygon to get attribute updates
  mPolygon.onEdited.attach(mOnEditedSlot);
}

PolygonGraphicsItem::~PolygonGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PolygonGraphicsItem::polygonEdited(const Polygon& polygon,
                                        Polygon::Event event) noexcept {
  switch (event) {
    case Polygon::Event::LayerNameChanged:
      setLineLayer(mLayerProvider.getLayer(*polygon.getLayerName()));
      updateFillLayer();  // required if the area is filled with the line layer
      break;
    case Polygon::Event::LineWidthChanged:
      setLineWidth(polygon.getLineWidth());
      break;
    case Polygon::Event::IsFilledChanged:
    case Polygon::Event::IsGrabAreaChanged:
      updateFillLayer();
      break;
    case Polygon::Event::PathChanged:
      setPath(polygon.getPath().toQPainterPathPx());
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PolygonGraphicsItem::polygonEdited()";
      break;
  }
}

void PolygonGraphicsItem::updateFillLayer() noexcept {
  if (mPolygon.isFilled()) {
    setFillLayer(mLayerProvider.getLayer(*mPolygon.getLayerName()));
  } else if (mPolygon.isGrabArea()) {
    setFillLayer(mLayerProvider.getGrabAreaLayer(*mPolygon.getLayerName()));
  } else {
    setFillLayer(nullptr);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
