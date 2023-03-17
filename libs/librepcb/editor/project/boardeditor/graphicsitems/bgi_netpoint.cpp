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

#include "../boardgraphicsscene.h"

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>

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

BGI_NetPoint::BGI_NetPoint(BI_NetPoint& netpoint) noexcept
  : QGraphicsItem(),
    mNetPoint(netpoint),
    mLayer(nullptr),
    mOnEditedSlot(*this, &BGI_NetPoint::netPointEdited),
    mOnLayerEditedSlot(*this, &BGI_NetPoint::layerEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  updateLayer();
  updatePosition();
  updateDiameter();
  updateNetSignalName();

  mNetPoint.onEdited.attach(mOnEditedSlot);
}

BGI_NetPoint::~BGI_NetPoint() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_NetPoint::shape() const noexcept {
  return (mLayer && mLayer->isVisible()) ? mShape : QPainterPath();
}

void BGI_NetPoint::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_NetPoint::netPointEdited(const BI_NetPoint& obj,
                                  BI_NetPoint::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_NetPoint::Event::PositionChanged:
      updatePosition();
      break;
    case BI_NetPoint::Event::LayerOfTracesChanged:
      updateLayer();
      break;
    case BI_NetPoint::Event::MaxTraceWidthChanged:
      updateDiameter();
      break;
    case BI_NetPoint::Event::NetSignalNameChanged:
      updateNetSignalName();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_NetPoint::netPointEdited():"
                 << static_cast<int>(event);
      break;
  }
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

void BGI_NetPoint::updateLayer() noexcept {
  // Set Z value.
  const QString layer = mNetPoint.getLayerOfLines();
  setZValue(layer.isEmpty()
                ? static_cast<qreal>(BoardGraphicsScene::ZValue_Default)
                : BoardGraphicsScene::getZValueOfCopperLayer(layer));

  // Set layer.
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = getLayer(layer);
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateVisibility();
}

void BGI_NetPoint::updatePosition() noexcept {
  setPos(mNetPoint.getPosition().toPxQPointF());
}

void BGI_NetPoint::updateDiameter() noexcept {
  prepareGeometryChange();
  if (mNetPoint.getMaxTraceWidth() > 0) {
    mShape = Path::circle(PositiveLength(*mNetPoint.getMaxTraceWidth()))
                 .toQPainterPathPx();
  } else {
    mShape = QPainterPath();
  }
  mBoundingRect = mShape.boundingRect();
  update();
}

void BGI_NetPoint::updateNetSignalName() noexcept {
  setToolTip(mNetPoint.getNetSegment().getNetNameToDisplay(true));
}

void BGI_NetPoint::updateVisibility() noexcept {
  setVisible(mLayer && mLayer->isVisible());
}

GraphicsLayer* BGI_NetPoint::getLayer(const QString& name) const noexcept {
  return mNetPoint.getBoard().getLayerStack().getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
