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
    mState(GraphicsLayer::State::Enabled),
    mLineWidthPx(0),
    mOnLayerEditedSlot(*this, &LineGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

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
  mLineWidthPx = width->toPx();
  updateBoundingRectAndShape();
}

void LineGraphicsItem::setLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = layer;
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
    setVisible(mLayer->isVisible());
  } else {
    setVisible(false);
  }
  update();
}

void LineGraphicsItem::setState(GraphicsLayer::State state) noexcept {
  mState = state;
  update();
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

  if ((!mLayer) || (!mLayer->isVisible())) {
    return;
  }

  GraphicsLayer::State state = mState;
  if (option->state.testFlag(QStyle::State_Selected)) {
    state = GraphicsLayer::State::Highlighted;
  }

  painter->setPen(
      QPen(mLayer->getColor(state), mLineWidthPx, Qt::SolidLine, Qt::RoundCap));
  painter->drawLine(mLine);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LineGraphicsItem::layerEdited(const GraphicsLayer& layer,
                                   GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
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
  mBoundingRect = Toolbox::adjustedBoundingRect(lineRect, mLineWidthPx / 2);
  // TODO: Should we also update the shape?
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
