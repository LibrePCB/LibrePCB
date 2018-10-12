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
#include "origincrossgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OriginCrossGraphicsItem::OriginCrossGraphicsItem(QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent), mLayer(nullptr), mSize(0) {
  mPen.setWidth(0);
  mPenHighlighted.setWidth(0);
  updateBoundingRectAndShape();
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setVisible(false);
}

OriginCrossGraphicsItem::~OriginCrossGraphicsItem() noexcept {
  // unregister from graphics layer
  setLayer(nullptr);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void OriginCrossGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void OriginCrossGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void OriginCrossGraphicsItem::setSize(const UnsignedLength& size) noexcept {
  mSize          = size;
  qreal lengthPx = size->toPx() / qreal(2);
  mLineH.setLine(-lengthPx, qreal(0), lengthPx, qreal(0));
  mLineV.setLine(qreal(0), -lengthPx, qreal(0), lengthPx);
  updateBoundingRectAndShape();
}

void OriginCrossGraphicsItem::setLayer(const GraphicsLayer* layer) noexcept {
  if (mLayer) {
    mLayer->unregisterObserver(*this);
  }
  mLayer = layer;
  if (mLayer) {
    mLayer->registerObserver(*this);
    mPen.setColor(mLayer->getColor(false));
    mPenHighlighted.setColor(mLayer->getColor(true));
    setVisible(mLayer->isVisible());
  } else {
    setVisible(false);
  }
}

/*******************************************************************************
 *  Inherited from IF_LayerObserver
 ******************************************************************************/

void OriginCrossGraphicsItem::layerColorChanged(
    const GraphicsLayer& layer, const QColor& newColor) noexcept {
  Q_UNUSED(layer);
  Q_ASSERT(&layer == mLayer);
  mPen.setColor(newColor);
  update();
}

void OriginCrossGraphicsItem::layerHighlightColorChanged(
    const GraphicsLayer& layer, const QColor& newColor) noexcept {
  Q_UNUSED(layer);
  Q_ASSERT(&layer == mLayer);
  mPenHighlighted.setColor(newColor);
  update();
}

void OriginCrossGraphicsItem::layerVisibleChanged(const GraphicsLayer& layer,
                                                  bool newVisible) noexcept {
  Q_ASSERT(&layer == mLayer);
  Q_UNUSED(newVisible);
  setVisible(layer.isVisible());
}

void OriginCrossGraphicsItem::layerEnabledChanged(const GraphicsLayer& layer,
                                                  bool newEnabled) noexcept {
  Q_ASSERT(&layer == mLayer);
  layerVisibleChanged(layer, newEnabled);
}

void OriginCrossGraphicsItem::layerDestroyed(
    const GraphicsLayer& layer) noexcept {
  Q_ASSERT(&layer == mLayer);
  setLayer(nullptr);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void OriginCrossGraphicsItem::paint(QPainter*                       painter,
                                    const QStyleOptionGraphicsItem* option,
                                    QWidget* widget) noexcept {
  Q_UNUSED(widget);
  if (option->state.testFlag(QStyle::State_Selected)) {
    painter->setPen(mPenHighlighted);
  } else {
    painter->setPen(mPen);
  }
  painter->drawLine(mLineH);
  painter->drawLine(mLineV);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OriginCrossGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  mBoundingRect = QRectF(-mSize->toPx() / 2, -mSize->toPx() / 2, mSize->toPx(),
                         mSize->toPx());
  mShape        = QPainterPath();
  mShape.addRect(mBoundingRect);
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
