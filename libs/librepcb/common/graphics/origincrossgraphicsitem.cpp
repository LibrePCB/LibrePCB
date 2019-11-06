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

OriginCrossGraphicsItem::OriginCrossGraphicsItem(QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLayer(nullptr),
    mSize(0),
    mVisibleInPrintOutput(false),  // don't show origin crosses by default
    mOnLayerEditedSlot(*this, &OriginCrossGraphicsItem::layerEdited) {
  mPen.setWidth(0);
  mPenHighlighted.setWidth(0);
  updateBoundingRectAndShape();
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setVisible(false);
}

OriginCrossGraphicsItem::~OriginCrossGraphicsItem() noexcept {
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
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = layer;
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
    mPen.setColor(mLayer->getColor(false));
    mPenHighlighted.setColor(mLayer->getColor(true));
    setVisible(mLayer->isVisible());
  } else {
    setVisible(false);
  }
}

void OriginCrossGraphicsItem::setVisibleInPrintOutput(bool visible) noexcept {
  mVisibleInPrintOutput = visible;
  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void OriginCrossGraphicsItem::paint(QPainter*                       painter,
                                    const QStyleOptionGraphicsItem* option,
                                    QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);
  const bool deviceIsPrinter =
      (dynamic_cast<QPrinter*>(painter->device()) != nullptr);

  if (deviceIsPrinter && (!mVisibleInPrintOutput)) {
    return;
  }

  QPen pen = isSelected ? mPenHighlighted : mPen;

  // When printing, enforce a minimum line width to make sure the line will be
  // visible (too thin lines will not be visible).
  qreal minPrintLineWidth = Length(100000).toPx();
  if (deviceIsPrinter && (pen.widthF() < minPrintLineWidth)) {
    pen.setWidthF(minPrintLineWidth);
  }

  painter->setPen(pen);
  painter->drawLine(mLineH);
  painter->drawLine(mLineV);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OriginCrossGraphicsItem::layerEdited(const GraphicsLayer& layer,
                                          GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      mPen.setColor(layer.getColor(false));
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      mPenHighlighted.setColor(layer.getColor(true));
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      setVisible(layer.isVisible() && layer.isEnabled());
      break;
    case GraphicsLayer::Event::Destroyed:
      setLayer(nullptr);
      break;
    default:
      qWarning()
          << "Unhandled switch-case in OriginCrossGraphicsItem::layerEdited()";
      break;
  }
}

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
