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
#include "primitivecirclegraphicsitem.h"

#include "../toolbox.h"

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

PrimitiveCircleGraphicsItem::PrimitiveCircleGraphicsItem(
    QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLineLayer(nullptr),
    mFillLayer(nullptr),
    mOnLayerEditedSlot(*this, &PrimitiveCircleGraphicsItem::layerEdited) {
  mPen.setWidthF(0);
  mPenHighlighted.setWidthF(0);
  updateColors();
  updateBoundingRectAndShape();
  updateVisibility();
}

PrimitiveCircleGraphicsItem::~PrimitiveCircleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitiveCircleGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitiveCircleGraphicsItem::setDiameter(
    const UnsignedLength& dia) noexcept {
  mCircleRect = Toolbox::boundingRectFromRadius(dia->toPx() / 2);
  updateBoundingRectAndShape();
}

void PrimitiveCircleGraphicsItem::setLineWidth(
    const UnsignedLength& width) noexcept {
  mPen.setWidthF(width->toPx());
  mPenHighlighted.setWidthF(width->toPx());
  updateBoundingRectAndShape();
}

void PrimitiveCircleGraphicsItem::setLineLayer(
    const GraphicsLayer* layer) noexcept {
  if (mLineLayer) {
    mLineLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLineLayer = layer;
  if (mLineLayer) {
    mLineLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateColors();
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitiveCircleGraphicsItem::setFillLayer(
    const GraphicsLayer* layer) noexcept {
  if (mFillLayer) {
    mFillLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mFillLayer = layer;
  if (mFillLayer) {
    mFillLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateColors();
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void PrimitiveCircleGraphicsItem::paint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option,
                                        QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);
  const bool deviceIsPrinter =
      (dynamic_cast<QPrinter*>(painter->device()) != nullptr);

  QPen pen = isSelected ? mPenHighlighted : mPen;
  QBrush brush = isSelected ? mBrushHighlighted : mBrush;

  // When printing, enforce a minimum line width to make sure the line will be
  // visible (too thin lines will not be visible).
  qreal minPrintLineWidth = Length(100000).toPx();
  if (deviceIsPrinter && (pen.widthF() < minPrintLineWidth)) {
    pen.setWidthF(minPrintLineWidth);
  }

  painter->setPen(pen);
  painter->setBrush(brush);
  painter->drawEllipse(mCircleRect);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitiveCircleGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateColors();
      updateVisibility();
      break;
    case GraphicsLayer::Event::Destroyed:
      if (&layer == mLineLayer) {
        setLineLayer(nullptr);
      } else if (&layer == mFillLayer) {
        setFillLayer(nullptr);
      } else {
        Q_ASSERT(false);
      }
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitiveCircleGraphicsItem::layerEdited()";
      break;
  }
}

void PrimitiveCircleGraphicsItem::updateColors() noexcept {
  if (mLineLayer && mLineLayer->isVisible()) {
    mPen.setStyle(Qt::SolidLine);
    mPenHighlighted.setStyle(Qt::SolidLine);
    mPen.setColor(mLineLayer->getColor(false));
    mPenHighlighted.setColor(mLineLayer->getColor(true));
  } else {
    mPen.setStyle(Qt::NoPen);
    mPenHighlighted.setStyle(Qt::NoPen);
  }

  if (mFillLayer && mFillLayer->isVisible()) {
    mBrush.setStyle(Qt::SolidPattern);
    mBrushHighlighted.setStyle(Qt::SolidPattern);
    mBrush.setColor(mFillLayer->getColor(false));
    mBrushHighlighted.setColor(mFillLayer->getColor(true));
  } else {
    mBrush.setStyle(Qt::NoBrush);
    mBrushHighlighted.setStyle(Qt::NoBrush);
  }
  update();
}

void PrimitiveCircleGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  mBoundingRect = Toolbox::adjustedBoundingRect(mCircleRect, mPen.widthF() / 2);
  QPainterPath p;
  p.addEllipse(mCircleRect);
  mShape = Toolbox::shapeFromPath(p, mPen, mBrush, UnsignedLength(200000));
  update();
}

void PrimitiveCircleGraphicsItem::updateVisibility() noexcept {
  setVisible((mPen.style() != Qt::NoPen) || (mBrush.style() != Qt::NoBrush));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
