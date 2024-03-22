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

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/origincrossgraphicsitem.h"
#include "../../../graphics/primitivecirclegraphicsitem.h"
#include "../../../graphics/primitiveholegraphicsitem.h"
#include "../../../graphics/primitivepathgraphicsitem.h"
#include "../../../graphics/primitivezonegraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/transform.h>
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

BGI_Device::BGI_Device(BI_Device& device,
                       const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItemGroup(),
    onEdited(*this),
    mDevice(device),
    mLayerProvider(lp),
    mGrabAreaLayer(nullptr),
    mOnEditedSlot(*this, &BGI_Device::deviceEdited),
    mOnLayerEditedSlot(*this, &BGI_Device::layerEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mOriginCrossGraphicsItem = std::make_shared<OriginCrossGraphicsItem>(this);
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1400000));

  for (auto& obj : mDevice.getLibFootprint().getCircles()) {
    auto i = std::make_shared<PrimitiveCircleGraphicsItem>(this);
    i->setPosition(obj.getCenter());
    i->setDiameter(positiveToUnsigned(obj.getDiameter()));
    i->setLineWidth(obj.getLineWidth());
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj.isGrabArea()) {
      const qreal r = (obj.getDiameter() + obj.getLineWidth())->toPx() / 2;
      QPainterPath path;
      path.addEllipse(obj.getCenter().toPxQPointF(), r, r);
      mShape |= path;
    }
    mCircleGraphicsItems.append(i);
  }

  for (auto& obj : mDevice.getLibFootprint().getPolygons()) {
    auto i = std::make_shared<PrimitivePathGraphicsItem>(this);
    i->setPath(obj.getPathForRendering().toQPainterPathPx());
    i->setLineWidth(obj.getLineWidth());
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj.isGrabArea()) {
      mShape |= Toolbox::shapeFromPath(obj.getPath().toQPainterPathPx(),
                                       QPen(Qt::SolidPattern, 0),
                                       Qt::SolidPattern, obj.getLineWidth());
    }
    mPolygonGraphicsItems.append(i);
  }

  for (auto& obj : mDevice.getLibFootprint().getZones()) {
    auto i = std::make_shared<PrimitiveZoneGraphicsItem>(mLayerProvider, this);
    i->setOutline(obj.getOutline());
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    mZoneGraphicsItems.append(i);
  }

  for (auto& obj : mDevice.getLibFootprint().getHoles()) {
    Q_UNUSED(obj);
    auto i = std::make_shared<PrimitiveHoleGraphicsItem>(mLayerProvider, false,
                                                         this);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    mHoleGraphicsItems.append(i);
  }

  updatePosition();
  updateRotationAndMirrored();
  updateBoardSide();
  updateHoleStopMaskOffsets();

  mDevice.onEdited.attach(mOnEditedSlot);
}

BGI_Device::~BGI_Device() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Device::shape() const noexcept {
  QPainterPath p = mOriginCrossGraphicsItem->shape();
  if (mGrabAreaLayer && mGrabAreaLayer->isVisible()) {
    p |= mShape;
  }
  return p;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Device::deviceEdited(const BI_Device& obj,
                              BI_Device::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Device::Event::BoardLayersChanged:
      updateZoneLayers();
      break;
    case BI_Device::Event::PositionChanged:
      updatePosition();
      break;
    case BI_Device::Event::RotationChanged:
      updateRotationAndMirrored();
      break;
    case BI_Device::Event::MirroredChanged:
      updateRotationAndMirrored();
      updateBoardSide();
      break;
    case BI_Device::Event::StopMaskOffsetsChanged:
      updateHoleStopMaskOffsets();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Device::deviceEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Device::layerEdited(const GraphicsLayer& layer,
                             GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
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

QVariant BGI_Device::itemChange(GraphicsItemChange change,
                                const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mOriginCrossGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
    foreach (const auto& i, mCircleGraphicsItems) {
      i->setSelected(value.toBool());
    }
    foreach (const auto& i, mPolygonGraphicsItems) {
      i->setSelected(value.toBool());
    }
    foreach (const auto& i, mZoneGraphicsItems) {
      i->setSelected(value.toBool());
    }
    foreach (const auto& i, mHoleGraphicsItems) {
      i->setSelected(value.toBool());
    }
    onEdited.notify(Event::SelectionChanged);
  }
  return QGraphicsItem::itemChange(change, value);
}

void BGI_Device::updatePosition() noexcept {
  setPos(mDevice.getPosition().toPxQPointF());
  onEdited.notify(Event::PositionChanged);
}

void BGI_Device::updateRotationAndMirrored() noexcept {
  QTransform t;
  t.rotate(-mDevice.getRotation().toDeg());
  if (mDevice.getMirrored()) t.scale(qreal(-1), qreal(1));
  setTransform(t);
}

void BGI_Device::updateBoardSide() noexcept {
  // Update Z value.
  const bool top = !mDevice.getMirrored();
  if (top) {
    setZValue(BoardGraphicsScene::ZValue_DevicesTop);
  } else {
    setZValue(BoardGraphicsScene::ZValue_DevicesBottom);
  }

  // Update grab area layer.
  std::shared_ptr<GraphicsLayer> grabAreaLayer =
      mLayerProvider.getLayer(top ? Theme::Color::sBoardGrabAreasTop
                                  : Theme::Color::sBoardGrabAreasBot);
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
  mOriginCrossGraphicsItem->setLayer(
      mLayerProvider.getLayer(top ? Theme::Color::sBoardReferencesTop
                                  : Theme::Color::sBoardReferencesBot));

  // Update circle layers.
  const CircleList& circles = mDevice.getLibFootprint().getCircles();
  for (int i = 0;
       i < std::min(circles.count(), int(mCircleGraphicsItems.count())); ++i) {
    mCircleGraphicsItems.at(i)->setLineLayer(
        getLayer(circles.at(i)->getLayer()));
    if (circles.at(i)->isFilled()) {
      mCircleGraphicsItems.at(i)->setFillLayer(
          getLayer(circles.at(i)->getLayer()));
    } else if (circles.at(i)->isGrabArea()) {
      mCircleGraphicsItems.at(i)->setFillLayer(mGrabAreaLayer);
    }
  }

  // Update polygon layers.
  const PolygonList& polygons = mDevice.getLibFootprint().getPolygons();
  for (int i = 0;
       i < std::min(polygons.count(), int(mPolygonGraphicsItems.count()));
       ++i) {
    mPolygonGraphicsItems.at(i)->setLineLayer(
        getLayer(polygons.at(i)->getLayer()));
    // Don't fill if path is not closed (for consistency with Gerber export)!
    if (polygons.at(i)->isFilled() && polygons.at(i)->getPath().isClosed()) {
      mPolygonGraphicsItems.at(i)->setFillLayer(
          getLayer(polygons.at(i)->getLayer()));
    } else if (polygons.at(i)->isGrabArea()) {
      mPolygonGraphicsItems.at(i)->setFillLayer(mGrabAreaLayer);
    }
  }

  // Update zone layers.
  updateZoneLayers();
}

void BGI_Device::updateHoleStopMaskOffsets() noexcept {
  for (int i = 0; i < std::min(mDevice.getLibFootprint().getHoles().count(),
                               int(mHoleGraphicsItems.count()));
       ++i) {
    const auto hole = mDevice.getLibFootprint().getHoles().at(i);
    const auto item = mHoleGraphicsItems.at(i);
    Q_ASSERT(hole && item);
    const auto stopMaskOffset =
        mDevice.getHoleStopMasks().value(hole->getUuid());
    item->setHole(hole->getPath(), hole->getDiameter(), stopMaskOffset);
  }
}

void BGI_Device::updateZoneLayers() noexcept {
  const Transform transform(mDevice);
  const ZoneList& zones = mDevice.getLibFootprint().getZones();
  for (int i = 0; i < std::min(zones.count(), int(mZoneGraphicsItems.count()));
       ++i) {
    mZoneGraphicsItems.at(i)->setAllLayers(
        mDevice.getBoard().getCopperLayers());
    QSet<const Layer*> enabledLayers;
    if (zones.at(i)->getLayers().testFlag(Zone::Layer::Top)) {
      enabledLayers.insert(&transform.map(Layer::topCopper()));
    }
    if (zones.at(i)->getLayers().testFlag(Zone::Layer::Inner)) {
      foreach (const Layer* layer, mDevice.getBoard().getCopperLayers()) {
        if (layer->isInner()) {
          enabledLayers.insert(layer);
        }
      }
    }
    if (zones.at(i)->getLayers().testFlag(Zone::Layer::Bottom)) {
      enabledLayers.insert(&transform.map(Layer::botCopper()));
    }
    mZoneGraphicsItems.at(i)->setEnabledLayers(enabledLayers);
  }
}

std::shared_ptr<GraphicsLayer> BGI_Device::getLayer(
    const Layer& layer) const noexcept {
  return mLayerProvider.getLayer(mDevice.getMirrored()
                                     ? layer.mirrored().getThemeColor()
                                     : layer.getThemeColor());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
