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
#include "bgi_polygon.h"

#include "../../../graphics/polygongraphicsitem.h"
#include "../boardgraphicsscene.h"

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

BGI_Polygon::BGI_Polygon(BI_Polygon& polygon,
                         const GraphicsLayerList& layers) noexcept
  : QGraphicsItemGroup(),
    mPolygon(polygon),
    mPolygonObj(polygon.getData().getUuid(), polygon.getData().getLayer(),
                polygon.getData().getLineWidth(), polygon.getData().isFilled(),
                polygon.getData().isGrabArea(), polygon.getData().getPath()),
    mGraphicsItem(new PolygonGraphicsItem(mPolygonObj, layers, this)),
    mOnEditedSlot(*this, &BGI_Polygon::polygonEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  updateZValue();
  updateEditable();

  mPolygon.onEdited.attach(mOnEditedSlot);
}

BGI_Polygon::~BGI_Polygon() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Polygon::shape() const noexcept {
  return mGraphicsItem->shape();
}

QVariant BGI_Polygon::itemChange(GraphicsItemChange change,
                                 const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    mGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Polygon::polygonEdited(const BI_Polygon& obj,
                                BI_Polygon::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Polygon::Event::LayerChanged:
      mPolygonObj.setLayer(obj.getData().getLayer());
      break;
    case BI_Polygon::Event::LineWidthChanged:
      mPolygonObj.setLineWidth(obj.getData().getLineWidth());
      break;
    case BI_Polygon::Event::IsFilledChanged:
      mPolygonObj.setIsFilled(obj.getData().isFilled());
      break;
    case BI_Polygon::Event::IsGrabAreaChanged:
      mPolygonObj.setIsGrabArea(obj.getData().isGrabArea());
      break;
    case BI_Polygon::Event::IsLockedChanged:
      updateEditable();
      break;
    case BI_Polygon::Event::PathChanged:
      mPolygonObj.setPath(obj.getData().getPath());
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Polygon::polygonEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Polygon::updateZValue() noexcept {
  setZValue(mPolygon.getData().getLayer().isBottom()
                ? BoardGraphicsScene::ZValue_PolygonsBottom
                : BoardGraphicsScene::ZValue_PolygonsTop);
}

void BGI_Polygon::updateEditable() noexcept {
  mGraphicsItem->setEditable(!mPolygon.getData().isLocked());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
