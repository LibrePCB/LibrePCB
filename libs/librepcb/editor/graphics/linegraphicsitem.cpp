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
#include "linegraphicsitem.h"

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

LineGraphicsItem::LineGraphicsItem(QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLayer(nullptr),
    mOnLayerEditedSlot(*this, &LineGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mPen.setCapStyle(Qt::RoundCap);
  mPenHighlighted.setCapStyle(Qt::RoundCap);
  mPen.setWidth(0);
  mPenHighlighted.setWidth(0);
  updateBoundingRectAndShape();
  setVisible(false);
}

LineGraphicsItem::~LineGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LineGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void LineGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void LineGraphicsItem::setLine(const Point& p1, const Point& p2) noexcept {
  mLine.setPoints(p1.toPxQPointF(), p2.toPxQPointF());
  updateBoundingRectAndShape();
}

void LineGraphicsItem::setLineWidth(const UnsignedLength& width) noexcept {
  mPen.setWidthF(width->toPx());
  mPenHighlighted.setWidthF(width->toPx());
  updateBoundingRectAndShape();
}

void LineGraphicsItem::setLayer(
    const std::shared_ptr<GraphicsLayer>& layer) noexcept {
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

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath LineGraphicsItem::shape() const noexcept {
  return (mLayer && mLayer->isVisible()) ? mShape : QPainterPath();
}

void LineGraphicsItem::paint(QPainter* painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget* widget) noexcept {
  Q_UNUSED(widget);
  if (option->state.testFlag(QStyle::State_Selected)) {
    painter->setPen(mPenHighlighted);
  } else {
    painter->setPen(mPen);
  }
  painter->drawLine(mLine);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LineGraphicsItem::layerEdited(const GraphicsLayer& layer,
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
    default:
      qWarning() << "Unhandled switch-case in LineGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void LineGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  QRectF lineRect(mLine.p1(), mLine.p2());
  mBoundingRect = Toolbox::adjustedBoundingRect(lineRect, mPen.widthF() / 2);
  // TODO: Should we also update the shape?
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
