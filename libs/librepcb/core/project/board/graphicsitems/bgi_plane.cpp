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

#include "../../../geometry/polygon.h"
#include "../../../graphics/primitivepathgraphicsitem.h"
#include "../../../utils/toolbox.h"
#include "../../project.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_plane.h"

#include <QPrinter>
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_Plane::BGI_Plane(BI_Plane& plane) noexcept
  : BGI_Base(),
    mPlane(plane),
    mLayer(nullptr),
    mLineWidthPx(0),
    mVertexRadiusPx(0),
    mOnLayerEditedSlot(*this, &BGI_Plane::layerEdited) {
  updateCacheAndRepaint();
}

BGI_Plane::~BGI_Plane() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_Plane::isSelectable() const noexcept {
  return mLayer && mLayer->isVisible();
}

int BGI_Plane::getLineIndexAtPosition(const Point& pos) const noexcept {
  // We build temporary PrimitivePathGraphicsItem objects for each segment of
  // the plane and check if the specified position is located within the shape
  // of one of these graphics items. This is quite ugly, but was easy to
  // implement and seems to work nicely... ;-)
  for (int i = 1; i < mPlane.getOutline().getVertices().count(); ++i) {
    Path path;
    path.addVertex(mPlane.getOutline().getVertices()[i - 1]);
    path.addVertex(mPlane.getOutline().getVertices()[i]);

    PrimitivePathGraphicsItem item(const_cast<BGI_Plane*>(this));
    item.setPath(path.toQPainterPathPx());
    item.setLineWidth(UnsignedLength(Length::fromPx(mLineWidthPx)));
    item.setLineLayer(mLayer);

    if (item.shape().contains(item.mapFromScene(pos.toPxQPointF()))) {
      return i;
    }
  }

  return -1;
}

QVector<int> BGI_Plane::getVertexIndicesAtPosition(const Point& pos) const
    noexcept {
  QVector<int> indices;
  for (int i = 0; i < mPlane.getOutline().getVertices().count(); ++i) {
    Point diff = (mPlane.getOutline().getVertices()[i].getPos() - pos);
    if (diff.getLength()->toPx() < mVertexRadiusPx) {
      indices.append(i);
    }
  }
  return indices;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_Plane::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  setZValue(getZValueOfCopperLayer(*mPlane.getLayerName()));

  // set layer
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = getLayer(*mPlane.getLayerName());
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateVisibility();

  // set shape and bounding rect
  mOutline = mPlane.getOutline().toClosedPath().toQPainterPathPx();
  mShape = mShape = Toolbox::shapeFromPath(
      mOutline, QPen(Length::fromMm(0.3).toPx()), QBrush());
  mBoundingRect = mShape.boundingRect();

  // get areas
  mAreas.clear();
  for (const Path& r : mPlane.getFragments()) {
    mAreas.append(r.toQPainterPathPx());
    mBoundingRect = mBoundingRect.united(mAreas.last().boundingRect());
  }

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_Plane::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget* widget) {
  Q_UNUSED(widget);

  const bool selected = mPlane.isSelected();
  const bool deviceIsPrinter =
      (dynamic_cast<QPrinter*>(painter->device()) != nullptr);
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  if (mLayer && mLayer->isVisible()) {
    // draw outline only on screen, not for print or PDF export
    if (!deviceIsPrinter) {
      mLineWidthPx = 3 / lod;
      painter->setPen(QPen(mLayer->getColor(selected), mLineWidthPx,
                           Qt::DashLine, Qt::RoundCap));
      painter->setBrush(Qt::NoBrush);
      painter->drawPath(mOutline);

      // if the plane is selected, draw vertex handles
      if (selected) {
        mVertexRadiusPx = (mLineWidthPx / 2) + Length::fromMm(0.2).toPx();
        painter->setPen(
            QPen(mLayer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));
        foreach (const Vertex& vertex, mPlane.getOutline().getVertices()) {
          painter->drawEllipse(vertex.getPos().toPxQPointF(), mVertexRadiusPx,
                               mVertexRadiusPx);
        }
      }
    }

    // draw plane only if plane should be visible
    if (mPlane.isVisible()) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(mLayer->getColor(selected));
      foreach (const QPainterPath& area, mAreas) { painter->drawPath(area); }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

GraphicsLayer* BGI_Plane::getLayer(const QString& name) const noexcept {
  return mPlane.getBoard().getLayerStack().getLayer(name);
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

void BGI_Plane::updateVisibility() noexcept {
  setVisible(mLayer && mLayer->isVisible());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
