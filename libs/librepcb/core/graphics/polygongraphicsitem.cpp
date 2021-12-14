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
  updateVertexGraphicsItems();
  updateFillLayer();
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  // register to the polygon to get attribute updates
  mPolygon.onEdited.attach(mOnEditedSlot);
}

PolygonGraphicsItem::~PolygonGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

int PolygonGraphicsItem::getLineIndexAtPosition(const Point& pos) const
    noexcept {
  // We build temporary PrimitivePathGraphicsItem objects for each segment of
  // the polygon and check if the specified position is located within the shape
  // of one of these graphics items. This is quite ugly, but was easy to
  // implement and seems to work nicely... ;-)
  for (int i = 1; i < mPolygon.getPath().getVertices().count(); ++i) {
    Path path;
    path.addVertex(mPolygon.getPath().getVertices()[i - 1]);
    path.addVertex(mPolygon.getPath().getVertices()[i]);

    PrimitivePathGraphicsItem item(const_cast<PolygonGraphicsItem*>(this));
    item.setPath(path.toQPainterPathPx());
    item.setLineWidth(mPolygon.getLineWidth());
    item.setLineLayer(mLineLayer);

    if (item.shape().contains(item.mapFromScene(pos.toPxQPointF()))) {
      return i;
    }
  }

  return -1;
}

QVector<int> PolygonGraphicsItem::getVertexIndicesAtPosition(
    const Point& pos) const noexcept {
  QVector<int> indices;
  for (int i = 0; i < mVertexGraphicsItems.count(); ++i) {
    if (mVertexGraphicsItems[i]->shape().contains(
            mVertexGraphicsItems[i]->mapFromScene(pos.toPxQPointF()))) {
      indices.append(i);
    }
  }
  return indices;
}

QVariant PolygonGraphicsItem::itemChange(GraphicsItemChange change,
                                         const QVariant& value) noexcept {
  if (change == ItemSelectedChange) {
    for (int i = 0; i < mVertexGraphicsItems.count(); ++i) {
      mVertexGraphicsItems[i]->setVisible(value.toBool());
    }
  }
  return QGraphicsItem::itemChange(change, value);
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
      updateVertexGraphicsItems();
      break;
    case Polygon::Event::LineWidthChanged:
      setLineWidth(polygon.getLineWidth());
      updateVertexGraphicsItems();
      break;
    case Polygon::Event::IsFilledChanged:
    case Polygon::Event::IsGrabAreaChanged:
      updateFillLayer();
      break;
    case Polygon::Event::PathChanged:
      setPath(polygon.getPath().toQPainterPathPx());
      updateFillLayer();  // path "closed" might have changed
      updateVertexGraphicsItems();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PolygonGraphicsItem::polygonEdited()";
      break;
  }
}

void PolygonGraphicsItem::updateFillLayer() noexcept {
  // Don't fill if path is not closed (for consistency with Gerber export)!
  if (mPolygon.isFilled() && mPolygon.getPath().isClosed()) {
    setFillLayer(mLayerProvider.getLayer(*mPolygon.getLayerName()));
  } else if (mPolygon.isGrabArea()) {
    setFillLayer(mLayerProvider.getGrabAreaLayer(*mPolygon.getLayerName()));
  } else {
    setFillLayer(nullptr);
  }
}

void PolygonGraphicsItem::updateVertexGraphicsItems() noexcept {
  const Path& path = mPolygon.getPath();

  while (mVertexGraphicsItems.count() < path.getVertices().count()) {
    std::shared_ptr<PrimitivePathGraphicsItem> item =
        std::make_shared<PrimitivePathGraphicsItem>(this);
    item->setShapeMode(PrimitivePathGraphicsItem::ShapeMode::FILLED_OUTLINE);
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    mVertexGraphicsItems.append(item);
  }

  while (mVertexGraphicsItems.count() > path.getVertices().count()) {
    mVertexGraphicsItems.takeLast();
  }

  for (int i = 0; i < mVertexGraphicsItems.count(); ++i) {
    Length size = (mPolygon.getLineWidth() / 2) + Length::fromMm(0.2);
    if (i == 0) {
      // first vertex is rectangular to make visible where the path starts
      mVertexGraphicsItems[i]->setPath(
          Path::rect(Point(-size, -size), Point(size, size))
              .toQPainterPathPx());
    } else {
      // other vertices are round
      mVertexGraphicsItems[i]->setPath(
          Path::circle(PositiveLength(size * 2)).toQPainterPathPx());
    }
    mVertexGraphicsItems[i]->setLineLayer(mLineLayer);
    mVertexGraphicsItems[i]->setPos(
        path.getVertices()[i].getPos().toPxQPointF());
    mVertexGraphicsItems[i]->setZValue(zValue() + 0.1);
    mVertexGraphicsItems[i]->setVisible(isSelected());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
