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
#include "bgi_device.h"

#include "../../../library/pkg/footprint.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_device.h"

#include <librepcb/core/graphics/origincrossgraphicsitem.h>
#include <librepcb/core/graphics/primitivecirclegraphicsitem.h>
#include <librepcb/core/graphics/primitivepathgraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_Device::BGI_Device(BI_Device& device) noexcept
  : BGI_Base(),
    mDevice(device),
    mGrabAreaLayer(),
    mOnLayerEditedSlot(*this, &BGI_Device::layerEdited) {
  mOriginCrossGraphicsItem = std::make_shared<OriginCrossGraphicsItem>(this);
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1400000));
  mShape |= mOriginCrossGraphicsItem->shape();

  for (auto& obj : mDevice.getLibFootprint().getCircles().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<PrimitiveCircleGraphicsItem>(this);
    i->setPosition(obj->getCenter());
    i->setDiameter(positiveToUnsigned(obj->getDiameter()));
    i->setLineWidth(obj->getLineWidth());
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj->isGrabArea()) {
      const qreal r = (obj->getDiameter() + obj->getLineWidth())->toPx() / 2;
      QPainterPath path;
      path.addEllipse(obj->getCenter().toPxQPointF(), r, r);
      mShape |= path;
    }
    mCircleGraphicsItems.append(i);
  }

  for (auto& obj : mDevice.getLibFootprint().getPolygons().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<PrimitivePathGraphicsItem>(this);
    i->setPath(obj->getPath().toQPainterPathPx());
    i->setLineWidth(obj->getLineWidth());
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj->isGrabArea()) {
      mShape |= Toolbox::shapeFromPath(obj->getPath().toQPainterPathPx(),
                                       QPen(Qt::SolidPattern, 0),
                                       Qt::SolidPattern, obj->getLineWidth());
    }
    mPolygonGraphicsItems.append(i);
  }

  for (auto& obj : mDevice.getLibFootprint().getHoles().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<PrimitiveCircleGraphicsItem>(this);
    i->setPosition(obj->getPosition());
    i->setDiameter(positiveToUnsigned(obj->getDiameter()));
    i->setLineLayer(getLayer(GraphicsLayer::sBoardDrillsNpth));
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    mHoleGraphicsItems.append(i);
  }

  updateBoardSide();

  mBoundingRect = childrenBoundingRect();
}

BGI_Device::~BGI_Device() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BGI_Device::isSelectable() const noexcept {
  GraphicsLayer* layer = getLayer(GraphicsLayer::sTopReferences);
  return layer && layer->isVisible();
}

void BGI_Device::setSelected(bool selected) noexcept {
  mOriginCrossGraphicsItem->setSelected(selected);
  foreach (const auto& i, mCircleGraphicsItems) { i->setSelected(selected); }
  foreach (const auto& i, mPolygonGraphicsItems) { i->setSelected(selected); }
  QGraphicsItem::setSelected(selected);
}

void BGI_Device::updateBoardSide() noexcept {
  // Update Z value.
  if (mDevice.getMirrored()) {
    setZValue(Board::ZValue_DevicesBottom);
  } else {
    setZValue(Board::ZValue_DevicesTop);
  }

  // Update grab area layer.
  GraphicsLayer* grabAreaLayer = getLayer(GraphicsLayer::sTopGrabAreas);
  if (grabAreaLayer != mGrabAreaLayer) {
    if (mGrabAreaLayer) {
      mGrabAreaLayer->onEdited.detach(mOnLayerEditedSlot);
    }
    prepareGeometryChange();
    mGrabAreaLayer = grabAreaLayer;
    if (mGrabAreaLayer) {
      mGrabAreaLayer->onEdited.attach(mOnLayerEditedSlot);
    }
  }

  // Update origin cross layer.
  mOriginCrossGraphicsItem->setLayer(getLayer(GraphicsLayer::sTopReferences));

  // Update circle layers.
  const CircleList& circles = mDevice.getLibFootprint().getCircles();
  for (int i = 0; i < std::min(circles.count(), mCircleGraphicsItems.count());
       ++i) {
    mCircleGraphicsItems.at(i)->setLineLayer(
        getLayer(*circles.at(i)->getLayerName()));
    if (circles.at(i)->isFilled()) {
      mCircleGraphicsItems.at(i)->setFillLayer(
          getLayer(*circles.at(i)->getLayerName()));
    } else if (circles.at(i)->isGrabArea()) {
      mCircleGraphicsItems.at(i)->setFillLayer(mGrabAreaLayer);
    }
  }

  // Update polygon layers.
  const PolygonList& polygons = mDevice.getLibFootprint().getPolygons();
  for (int i = 0; i < std::min(polygons.count(), mPolygonGraphicsItems.count());
       ++i) {
    mPolygonGraphicsItems.at(i)->setLineLayer(
        getLayer(*polygons.at(i)->getLayerName()));
    // Don't fill if path is not closed (for consistency with Gerber export)!
    if (polygons.at(i)->isFilled() && polygons.at(i)->getPath().isClosed()) {
      mPolygonGraphicsItems.at(i)->setFillLayer(
          getLayer(*polygons.at(i)->getLayerName()));
    } else if (polygons.at(i)->isGrabArea()) {
      mPolygonGraphicsItems.at(i)->setFillLayer(mGrabAreaLayer);
    }
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Device::shape() const noexcept {
  return (mGrabAreaLayer && mGrabAreaLayer->isVisible())
      ? mShape
      : mOriginCrossGraphicsItem->shape();
}

void BGI_Device::paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option,
                       QWidget* widget) {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Device::layerEdited(const GraphicsLayer& layer,
                             GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
    case GraphicsLayer::Event::Destroyed:
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      prepareGeometryChange();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Device::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

GraphicsLayer* BGI_Device::getLayer(QString name) const noexcept {
  if (mDevice.getMirrored()) {
    name = GraphicsLayer::getMirroredLayerName(name);
  }
  return mDevice.getBoard().getLayerStack().getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
