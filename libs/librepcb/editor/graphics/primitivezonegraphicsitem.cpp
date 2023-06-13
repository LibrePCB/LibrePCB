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
#include "primitivezonegraphicsitem.h"

#include "graphicslayer.h"

#include <librepcb/core/geometry/path.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>

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

PrimitiveZoneGraphicsItem::PrimitiveZoneGraphicsItem(
    const IF_GraphicsLayerProvider& lp, QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLayerProvider(lp),
    mOutline(),
    mEditable(false),
    mLayer(lp.getLayer(Theme::Color::sBoardZones)),
    mBoundingRectMarginPx(0),
    mVertexHandleRadiusPx(0),
    mVertexHandles(),
    mOnLayerEditedSlot(*this, &PrimitiveZoneGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(-1);

  mPen.setCapStyle(Qt::RoundCap);
  mPenHighlighted.setCapStyle(Qt::RoundCap);
  mPen.setJoinStyle(Qt::RoundJoin);
  mPenHighlighted.setJoinStyle(Qt::RoundJoin);
  mPen.setWidthF(0);
  mPenHighlighted.setWidthF(0);

  updateColors();
  updateBoundingRectAndShape();
  updateVisibility();

  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
}

PrimitiveZoneGraphicsItem::~PrimitiveZoneGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int PrimitiveZoneGraphicsItem::getLineIndexAtPosition(const Point& pos) const
    noexcept {
  // We build temporary PrimitivePathGraphicsItem objects for each segment of
  // the zone and check if the specified position is located within the shape
  // of one of these graphics items. This is quite ugly, but was easy to
  // implement and seems to work nicely... ;-)
  const UnsignedLength width(Length::fromPx(mVertexHandleRadiusPx * 2));
  const Path outline = mOutline.toClosedPath();
  for (int i = 1; i < outline.getVertices().count(); ++i) {
    Path path;
    path.addVertex(outline.getVertices()[i - 1]);
    path.addVertex(outline.getVertices()[i]);

    PrimitivePathGraphicsItem item(
        const_cast<PrimitiveZoneGraphicsItem*>(this));
    item.setPath(path.toQPainterPathPx());
    item.setLineWidth(width);
    item.setLineLayer(mLayer);

    if (item.shape().contains(item.mapFromScene(pos.toPxQPointF()))) {
      return i;
    }
  }

  return -1;
}

QVector<int> PrimitiveZoneGraphicsItem::getVertexIndicesAtPosition(
    const Point& pos) const noexcept {
  QMultiMap<Length, int> indices;
  for (int i = 0; i < mOutline.getVertices().count(); ++i) {
    const Length distance =
        *(mOutline.getVertices().at(i).getPos() - pos).getLength();
    if (distance.toPx() <= mVertexHandleRadiusPx) {
      indices.insert(distance, i);
    }
  }
  return indices.values(indices.uniqueKeys().value(0)).toVector();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitiveZoneGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitiveZoneGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void PrimitiveZoneGraphicsItem::setAllLayers(
    const QSet<const Layer*>& layers) noexcept {
  foreach (auto layer, mAllGraphicsLayers) {
    layer->onEdited.detach(mOnLayerEditedSlot);
  }
  mAllGraphicsLayers.clear();
  foreach (const Layer* layer, layers) {
    if (auto graphicsLayer = mLayerProvider.getLayer(*layer)) {
      mAllGraphicsLayers.append(graphicsLayer);
      graphicsLayer->onEdited.attach(mOnLayerEditedSlot);
    }
  }

  updateVisibility();
}

void PrimitiveZoneGraphicsItem::setEnabledLayers(
    const QSet<const Layer*>& layers) noexcept {
  QVector<std::shared_ptr<GraphicsLayer>> graphicsLayers;
  foreach (const Layer* layer, layers) {
    if (auto graphicsLayer = mLayerProvider.getLayer(*layer)) {
      graphicsLayers.append(graphicsLayer);
    } else if (!layer) {
      graphicsLayers.append(nullptr);
    }
  }

  if (graphicsLayers != mEnabledGraphicsLayers) {
    mEnabledGraphicsLayers = graphicsLayers;
    updateVisibility();
  }
}

void PrimitiveZoneGraphicsItem::setOutline(const Path& path) noexcept {
  if (path == mOutline) {
    return;
  }

  mOutline = path;
  mVertexHandles.clear();
  const int count = mOutline.getVertices().count();
  for (int i = 0; i < count; ++i) {
    const Point p = mOutline.getVertices().at(i).getPos();
    Length maxRadius(10000000);
    for (int k = 0; k < count; ++k) {
      Length radius =
          (p - mOutline.getVertices().at(k).getPos()).getLength() / 2;
      if ((radius > 0) && (radius < maxRadius)) {
        maxRadius = radius;
      }
    }
    mVertexHandles.append(VertexHandle{p, maxRadius.toPx()});
  }
  mPainterPath = path.toClosedPath().toQPainterPathPx();
  updateBoundingRectAndShape();
  updateBoundingRectMargin();
}

void PrimitiveZoneGraphicsItem::setEditable(bool editable) noexcept {
  mEditable = editable;
  updateBoundingRectMargin();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QVariant PrimitiveZoneGraphicsItem::itemChange(GraphicsItemChange change,
                                               const QVariant& value) noexcept {
  if (change == ItemSelectedHasChanged) {
    updateBoundingRectMargin();
  }
  return QGraphicsItem::itemChange(change, value);
}

QPainterPath PrimitiveZoneGraphicsItem::shape() const noexcept {
  return isVisible() ? mShape : QPainterPath();
}

void PrimitiveZoneGraphicsItem::paint(QPainter* painter,
                                      const QStyleOptionGraphicsItem* option,
                                      QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  // Draw outline & fill.
  painter->setPen(isSelected ? mPenHighlighted : mPen);
  painter->setBrush(isSelected ? mBrushHighlighted : mBrush);
  painter->drawPath(mPainterPath);

  // Draw vertex handles, if editable and selected.
  if (mEditable && isSelected && mLayer) {
    const qreal radius = 20 / lod;
    mVertexHandleRadiusPx = std::min(radius, mBoundingRectMarginPx);
    QColor color = mLayer->getColor(isSelected);
    color.setAlpha(color.alpha() / 3);
    painter->setBrush(Qt::NoBrush);
    for (int i = 0; i < mVertexHandles.count(); ++i) {
      const QPointF p = mVertexHandles.at(i).pos.toPxQPointF();
      const qreal glowRadius =
          std::min(radius, mVertexHandles.at(i).maxGlowRadiusPx * 1.5);
      QRadialGradient gradient(p, glowRadius);
      gradient.setColorAt(0, color);
      gradient.setColorAt(0.5, color);
      gradient.setColorAt(1, Qt::transparent);
      painter->setPen(QPen(QBrush(gradient), glowRadius * 2));
      painter->drawPoint(p);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitiveZoneGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateColors();
      updateVisibility();
      update();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitiveZoneGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PrimitiveZoneGraphicsItem::updateColors() noexcept {
  if (mLayer) {
    QColor color = mLayer->getColor(false);
    color.setAlpha(255);
    QColor colorHighlighted = mLayer->getColor(true);
    colorHighlighted.setAlpha(255);

    mPen.setStyle(Qt::SolidLine);
    mPenHighlighted.setStyle(Qt::SolidLine);
    mPen.setColor(color);
    mPenHighlighted.setColor(colorHighlighted);

    mBrush.setStyle(Qt::SolidPattern);
    mBrushHighlighted.setStyle(Qt::SolidPattern);
    mBrush.setColor(mLayer->getColor(false));
    mBrushHighlighted.setColor(mLayer->getColor(true));
  }

  update();
}

void PrimitiveZoneGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  mShape = Toolbox::shapeFromPath(mPainterPath, mPen, mBrush);
  mBoundingRect = mPainterPath.boundingRect() +
      QMarginsF(mPen.widthF(), mPen.widthF(), mPen.widthF(), mPen.widthF());
  update();
}

void PrimitiveZoneGraphicsItem::updateBoundingRectMargin() noexcept {
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

void PrimitiveZoneGraphicsItem::updateVisibility() noexcept {
  bool visible = false;
  if (mLayer && mLayer->isVisible()) {
    bool anyCopperLayerVisible = false;
    bool anyZoneLayerVisible = false;
    foreach (auto graphicsLayer, mAllGraphicsLayers) {
      if (graphicsLayer->isVisible()) {
        anyCopperLayerVisible = true;
        if (mEnabledGraphicsLayers.contains(graphicsLayer)) {
          anyZoneLayerVisible = true;
          break;
        }
      }
    }
    visible = anyZoneLayerVisible || (!anyCopperLayerVisible);
  }
  setVisible(visible);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
