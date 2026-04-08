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

PrimitiveCircleGraphicsItem::PrimitiveCircleGraphicsItem(
    QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLineLayer(nullptr),
    mFillLayer(nullptr),
    mState(GraphicsLayer::State::Enabled),
    mShapeMode(ShapeMode::StrokeAndAreaByLayer),
    mLineWidthPx(0),
    mOnLayerEditedSlot(*this, &PrimitiveCircleGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

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
  mLineWidthPx = width->toPx();
  updateBoundingRectAndShape();
}

void PrimitiveCircleGraphicsItem::setLineLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mLineLayer) {
    mLineLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLineLayer = layer;
  if (mLineLayer) {
    mLineLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitiveCircleGraphicsItem::setFillLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mFillLayer) {
    mFillLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mFillLayer = layer;
  if (mFillLayer) {
    mFillLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitiveCircleGraphicsItem::setShapeMode(ShapeMode mode) noexcept {
  mShapeMode = mode;
  updateBoundingRectAndShape();
}

void PrimitiveCircleGraphicsItem::setState(
    GraphicsLayer::State state) noexcept {
  mState = state;
  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitiveCircleGraphicsItem::shape() const noexcept {
  return ((mLineLayer && mLineLayer->isVisible()) ||
          (mFillLayer && mFillLayer->isVisible()))
      ? mShape
      : QPainterPath();
}

void PrimitiveCircleGraphicsItem::paint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option,
                                        QWidget* widget) noexcept {
  Q_UNUSED(widget);

  GraphicsLayer::State state = mState;
  if (option->state.testFlag(QStyle::State_Selected)) {
    state = GraphicsLayer::State::Highlighted;
  }

  painter->setPen(getPen(state));
  painter->setBrush(getBrush(state));
  painter->drawEllipse(mCircleRect);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitiveCircleGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateBoundingRectAndShape();  // Shape may have changed.
      updateVisibility();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitiveCircleGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PrimitiveCircleGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  QPainterPath p;
  p.addEllipse(mCircleRect);
  if (mShapeMode == ShapeMode::FilledOutline) {
    mShape = p;
  } else {
    mShape = Toolbox::shapeFromPath(p, getPen(mState), getBrush(mState));
  }
  mBoundingRect = mShape.controlPointRect();
  update();
}

void PrimitiveCircleGraphicsItem::updateVisibility() noexcept {
  setVisible((mLineLayer && mLineLayer->isVisible()) ||
             (mFillLayer && mFillLayer->isVisible()));
}

QPen PrimitiveCircleGraphicsItem::getPen(
    GraphicsLayer::State state) const noexcept {
  if (mLineLayer && mLineLayer->isVisible()) {
    return QPen(mLineLayer->getColor(state), mLineWidthPx, Qt::SolidLine);
  } else {
    return Qt::NoPen;
  }
}

QBrush PrimitiveCircleGraphicsItem::getBrush(
    GraphicsLayer::State state) const noexcept {
  if (mFillLayer && mFillLayer->isVisible()) {
    return QBrush(mFillLayer->getColor(state), Qt::SolidPattern);
  } else {
    return Qt::NoBrush;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
