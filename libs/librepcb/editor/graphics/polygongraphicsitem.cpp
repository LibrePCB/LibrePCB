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

#include "graphicslayer.h"

#include <librepcb/core/utils/toolbox.h>

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

PolygonGraphicsItem::PolygonGraphicsItem(Polygon& polygon,
                                         const IF_GraphicsLayerProvider& lp,
                                         QGraphicsItem* parent) noexcept
  : PrimitivePathGraphicsItem(parent),
    mPolygon(polygon),
    mLayerProvider(lp),
    mEditable(false),
    mVertexHandleRadiusPx(0),
    mVertexHandles(),
    mOnEditedSlot(*this, &PolygonGraphicsItem::polygonEdited) {
  setLineWidth(mPolygon.getLineWidth());
  setLineLayer(mLayerProvider.getLayer(mPolygon.getLayer()));
  updatePath();
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
  const UnsignedLength width(std::max(
      *mPolygon.getLineWidth(), Length::fromPx(mVertexHandleRadiusPx * 2)));
  for (int i = 1; i < mPolygon.getPath().getVertices().count(); ++i) {
    Path path;
    path.addVertex(mPolygon.getPath().getVertices()[i - 1]);
    path.addVertex(mPolygon.getPath().getVertices()[i]);

    PrimitivePathGraphicsItem item(const_cast<PolygonGraphicsItem*>(this));
    item.setPath(path.toQPainterPathPx());
    item.setLineWidth(width);
    item.setLineLayer(mLineLayer);

    if (item.shape().contains(item.mapFromScene(pos.toPxQPointF()))) {
      return i;
    }
  }

  return -1;
}

QVector<int> PolygonGraphicsItem::getVertexIndicesAtPosition(
    const Point& pos) const noexcept {
  QMultiMap<Length, int> indices;
  for (int i = 0; i < mPolygon.getPath().getVertices().count(); ++i) {
    const Length distance =
        *(mPolygon.getPath().getVertices().at(i).getPos() - pos).getLength();
    if (distance.toPx() <= mVertexHandleRadiusPx) {
      indices.insert(distance, i);
    }
  }
  return indices.values(indices.uniqueKeys().value(0)).toVector();
}

void PolygonGraphicsItem::setEditable(bool editable) noexcept {
  mEditable = editable;
  updateBoundingRectMargin();
}

QVariant PolygonGraphicsItem::itemChange(GraphicsItemChange change,
                                         const QVariant& value) noexcept {
  if (change == ItemSelectedHasChanged) {
    updateBoundingRectMargin();
  }
  return PrimitivePathGraphicsItem::itemChange(change, value);
}

void PolygonGraphicsItem::paint(QPainter* painter,
                                const QStyleOptionGraphicsItem* option,
                                QWidget* widget) noexcept {
  PrimitivePathGraphicsItem::paint(painter, option, widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  // Draw vertex handles, if editable and selected.
  if (mEditable && isSelected && mLineLayer) {
    const qreal radius = 20 / lod;
    mVertexHandleRadiusPx =
        std::min(std::max(radius, mPolygon.getLineWidth()->toPx() / 2),
                 mBoundingRectMarginPx);
    QColor color = mLineLayer->getColor(isSelected);
    color.setAlpha(color.alpha() / 3);
    painter->setBrush(Qt::NoBrush);
    for (int i = 0; i < mVertexHandles.count(); ++i) {
      const QPointF p = mVertexHandles.at(i).pos.toPxQPointF();
      const qreal glowRadius =
          std::max(std::min(radius, mVertexHandles.at(i).maxGlowRadiusPx * 1.5),
                   mPolygon.getLineWidth()->toPx() * 2);
      QRadialGradient gradient(p, glowRadius);
      gradient.setColorAt(0, color);
      gradient.setColorAt(0.5, color);
      gradient.setColorAt(1, Qt::transparent);
      painter->setPen(QPen(QBrush(gradient), glowRadius * 2));
      painter->drawPoint(p);
      if (mPolygon.getLineWidth() > 0) {
        const qreal lineRadius = mPolygon.getLineWidth()->toPx() / 2;
        QColor color(0, 0, 0, 80);
        painter->setPen(QPen(color, 0));
        painter->drawEllipse(p, lineRadius, lineRadius);
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PolygonGraphicsItem::polygonEdited(const Polygon& polygon,
                                        Polygon::Event event) noexcept {
  switch (event) {
    case Polygon::Event::LayerChanged:
      setLineLayer(mLayerProvider.getLayer(polygon.getLayer()));
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
      updatePath();
      updateFillLayer();  // path "closed" might have changed
      break;
    default:
      qWarning()
          << "Unhandled switch-case in PolygonGraphicsItem::polygonEdited():"
          << static_cast<int>(event);
      break;
  }
}

void PolygonGraphicsItem::updateFillLayer() noexcept {
  // Don't fill if path is not closed (for consistency with Gerber export)!
  if (mPolygon.isFilled() && mPolygon.getPath().isClosed()) {
    setFillLayer(mLayerProvider.getLayer(mPolygon.getLayer()));
  } else if (mPolygon.isGrabArea()) {
    setFillLayer(mLayerProvider.getGrabAreaLayer(mPolygon.getLayer()));
  } else {
    setFillLayer(nullptr);
  }
}

void PolygonGraphicsItem::updatePath() noexcept {
  mVertexHandles.clear();
  const int count = mPolygon.getPath().getVertices().count();
  for (int i = 0; i < count; ++i) {
    const Point p = mPolygon.getPath().getVertices().at(i).getPos();
    Length maxRadius(10000000);
    for (int k = 0; k < count; ++k) {
      Length radius =
          (p - mPolygon.getPath().getVertices().at(k).getPos()).getLength() / 2;
      if ((radius > 0) && (radius < maxRadius)) {
        maxRadius = radius;
      }
    }
    mVertexHandles.append(VertexHandle{p, maxRadius.toPx()});
  }
  setPath(mPolygon.getPath().toQPainterPathPx());
  updateBoundingRectMargin();
}

void PolygonGraphicsItem::updateBoundingRectMargin() noexcept {
  // Increase bounding rect by the maximum allowed vertex handle size if
  // the polygon is selected and editable, to include the vertex handles.
  // Otherwise remove the margin to avoid too much margin around the whole
  // graphics scene (e.g. leading to wrong zoom-all or graphics export scaling).
  prepareGeometryChange();
  mBoundingRectMarginPx = 0;
  if (mEditable && isSelected()) {
    foreach (const VertexHandle& handle, mVertexHandles) {
      mBoundingRectMarginPx =
          std::max(handle.maxGlowRadiusPx, mBoundingRectMarginPx);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
