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
#include "bgi_netpoint.h"

#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_netpoint.h"

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

BGI_NetPoint::BGI_NetPoint(BI_NetPoint& netpoint) noexcept
  : BGI_Base(),
    mNetPoint(netpoint),
    mLayer(nullptr),
    mOnLayerEditedSlot(*this, &BGI_NetPoint::layerEdited) {
  updateCacheAndRepaint();
}

BGI_NetPoint::~BGI_NetPoint() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_NetPoint::isSelectable() const noexcept {
  return mLayer && mLayer->isVisible();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_NetPoint::updateCacheAndRepaint() noexcept {
  setToolTip(*mNetPoint.getNetSignalOfNetSegment().getName());

  prepareGeometryChange();

  // set Z value
  GraphicsLayer* layer = mNetPoint.getLayerOfLines();
  setZValue(layer ? getZValueOfCopperLayer(layer->getName()) : 0);

  // set layer
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = mNetPoint.getLayerOfLines();
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateVisibility();

  qreal radius = mNetPoint.getMaxLineWidth()->toPx() / 2;
  mBoundingRect = QRectF(-radius, -radius, 2 * radius, 2 * radius);

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_NetPoint::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  bool highlight = mNetPoint.isSelected() ||
      mNetPoint.getNetSignalOfNetSegment().isHighlighted();

#ifdef QT_DEBUG
  GraphicsLayer* layer =
      getLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  Q_ASSERT(layer);
  if (layer->isVisible()) {
    // draw bounding rect
    painter->setPen(QPen(layer->getColor(highlight), 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());
  }
#else
  Q_UNUSED(highlight);
  Q_UNUSED(painter);
#endif
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

GraphicsLayer* BGI_NetPoint::getLayer(const QString& name) const noexcept {
  return mNetPoint.getBoard().getLayerStack().getLayer(name);
}

void BGI_NetPoint::layerEdited(const GraphicsLayer& layer,
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

void BGI_NetPoint::updateVisibility() noexcept {
  setVisible(mLayer && mLayer->isVisible());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
