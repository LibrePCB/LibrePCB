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
#include "bgi_plane.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/primitivepathgraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/types/layer.h>
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

BGI_Plane::BGI_Plane(BI_Plane& plane, const IF_GraphicsLayerProvider& lp,
                     std::shared_ptr<const QSet<const NetSignal*>>
                         highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mPlane(plane),
    mLayerProvider(lp),
    mHighlightedNetSignals(highlightedNetSignals),
    mLayer(nullptr),
    mBoundingRectMarginPx(0),
    mLineWidthPx(0),
    mVertexHandleRadiusPx(0),
    mVertexHandles(),
    mOnEditedSlot(*this, &BGI_Plane::planeEdited),
    mOnLayerEditedSlot(*this, &BGI_Plane::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  updateOutlineAndFragments();
  updateLayer();
  updateVisibility();

  mPlane.onEdited.attach(mOnEditedSlot);
}

BGI_Plane::~BGI_Plane() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int BGI_Plane::getLineIndexAtPosition(const Point& pos) const noexcept {
  // We build temporary PrimitivePathGraphicsItem objects for each segment of
  // the plane and check if the specified position is located within the shape
  // of one of these graphics items. This is quite ugly, but was easy to
  // implement and seems to work nicely... ;-)
  const UnsignedLength width(std::max(
      Length::fromPx(mLineWidthPx), Length::fromPx(mVertexHandleRadiusPx * 2)));
  const Path outline = mPlane.getOutline().toClosedPath();  // Add last segment.
  for (int i = 1; i < outline.getVertices().count(); ++i) {
    Path path;
    path.addVertex(outline.getVertices()[i - 1]);
    path.addVertex(outline.getVertices()[i]);

    PrimitivePathGraphicsItem item(const_cast<BGI_Plane*>(this));
    item.setPath(path.toQPainterPathPx());
    item.setLineWidth(width);
    item.setLineLayer(mLayer);

    if (item.shape().contains(item.mapFromScene(pos.toPxQPointF()))) {
      return i;
    }
  }

  return -1;
}

QVector<int> BGI_Plane::getVertexIndicesAtPosition(const Point& pos) const
    noexcept {
  QMultiMap<Length, int> indices;
  for (int i = 0; i < mPlane.getOutline().getVertices().count(); ++i) {
    const Length distance =
        *(mPlane.getOutline().getVertices().at(i).getPos() - pos).getLength();
    if (distance.toPx() <= mVertexHandleRadiusPx) {
      indices.insert(distance, i);
    }
  }
  return indices.values(indices.uniqueKeys().value(0)).toVector();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QVariant BGI_Plane::itemChange(GraphicsItemChange change,
                               const QVariant& value) noexcept {
  if (change == ItemSelectedHasChanged) {
    updateBoundingRectMargin();
  }
  return QGraphicsItem::itemChange(change, value);
}

QPainterPath BGI_Plane::shape() const noexcept {
  if ((!mLayer) || (!mLayer->isVisible())) {
    return QPainterPath();
  }

  const Length vertexHandleSize = Length::fromPx(mVertexHandleRadiusPx * 2);
  if ((vertexHandleSize > 0) && (isSelected())) {
    // Extend shape by vertex handles.
    return Toolbox::shapeFromPath(mOutline, QPen(Length::fromMm(0.3).toPx()),
                                  QBrush(), UnsignedLength(vertexHandleSize));
  } else {
    return mShape;
  }
}

void BGI_Plane::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool selected = option->state.testFlag(QStyle::State_Selected);
  const bool highlight =
      selected || mHighlightedNetSignals->contains(&mPlane.getNetSignal());
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  if (mLayer && mLayer->isVisible()) {
    // Draw outline.
    mLineWidthPx = 3 / lod;
    painter->setPen(QPen(mLayer->getColor(highlight), mLineWidthPx,
                         Qt::DashLine, Qt::RoundCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(mOutline);

    // If the plane is selected, draw vertex handles.
    if (selected) {
      const qreal radius = 20 / lod;
      mVertexHandleRadiusPx = std::min(radius, mBoundingRectMarginPx);
      QColor color = mLayer->getColor(highlight);
      color.setAlpha(color.alpha() / 2);
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

    // Draw plane only if plane should be visible.
    if (mPlane.isVisible()) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(mLayer->getColor(highlight));
      foreach (const QPainterPath& area, mAreas) { painter->drawPath(area); }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Plane::planeEdited(const BI_Plane& obj,
                            BI_Plane::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Plane::Event::OutlineChanged:
    case BI_Plane::Event::FragmentsChanged:
      updateOutlineAndFragments();
      break;
    case BI_Plane::Event::LayerChanged:
      updateLayer();
      updateVisibility();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Plane::planeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Plane::layerEdited(const GraphicsLayer& layer,
                            GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);

  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateVisibility();
      break;
    default:
      break;
  }
}

void BGI_Plane::updateOutlineAndFragments() noexcept {
  prepareGeometryChange();

  // calculate vertex handle sizes
  mVertexHandles.clear();
  const int count = mPlane.getOutline().getVertices().count();
  for (int i = 0; i < count; ++i) {
    const Point p = mPlane.getOutline().getVertices().at(i).getPos();
    Length maxRadius(10000000);
    for (int k = 0; k < count; ++k) {
      Length radius =
          (p - mPlane.getOutline().getVertices().at(k).getPos()).getLength() /
          2;
      if ((radius > 0) && (radius < maxRadius)) {
        maxRadius = radius;
      }
    }
    mVertexHandles.append(VertexHandle{p, maxRadius.toPx()});
  }

  // set shape and bounding rect
  mOutline = mPlane.getOutline().toClosedPath().toQPainterPathPx();
  mShape =
      Toolbox::shapeFromPath(mOutline, QPen(Qt::SolidPattern, 0), QBrush());
  mBoundingRect = mShape.boundingRect().adjusted(-5, -5, 10, 10);

  // get areas
  mAreas.clear();
  for (const Path& r : mPlane.getFragments()) {
    mAreas.append(r.toQPainterPathPx());
    mBoundingRect = mBoundingRect.united(mAreas.last().boundingRect());
  }

  updateBoundingRectMargin();
}

void BGI_Plane::updateLayer() noexcept {
  if (mPlane.getLayer() == Layer::topCopper()) {
    setZValue(BoardGraphicsScene::ZValue_PlanesTop);
  } else if (mPlane.getLayer() == Layer::botCopper()) {
    setZValue(BoardGraphicsScene::ZValue_PlanesBottom);
  } else {
    setZValue(BoardGraphicsScene::getZValueOfCopperLayer(mPlane.getLayer()));
  }

  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = mLayerProvider.getLayer(mPlane.getLayer());
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
}

void BGI_Plane::updateVisibility() noexcept {
  setVisible(mLayer && mLayer->isVisible());
  setSelected(isVisible() && isSelected());
}

void BGI_Plane::updateBoundingRectMargin() noexcept {
  // Increase bounding rect by the maximum allowed vertex handle size if
  // the polygon is selected and editable, to include the vertex handles.
  // Otherwise remove the margin to avoid too much margin around the whole
  // graphics scene (e.g. leading to wrong zoom-all or graphics export scaling).
  prepareGeometryChange();
  mBoundingRectMarginPx = 0;
  if (isSelected()) {
    foreach (const VertexHandle& handle, mVertexHandles) {
      mBoundingRectMarginPx =
          std::max(handle.maxGlowRadiusPx, mBoundingRectMarginPx);
    }
  }
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
