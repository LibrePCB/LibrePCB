/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "symbolpingraphicsitem.h"

#include "symbolpin.h"

#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/linegraphicsitem.h>
#include <librepcb/common/graphics/primitivetextgraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolPinGraphicsItem::SymbolPinGraphicsItem(SymbolPin& pin,
                                             const IF_GraphicsLayerProvider& lp,
                                             QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mPin(pin),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)) {
  setFlag(QGraphicsItem::ItemHasNoContents, false);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(10);

  // circle
  mCircleGraphicsItem->setDiameter(UnsignedLength(1200000));
  mCircleGraphicsItem->setLineLayer(
      lp.getLayer(GraphicsLayer::sSymbolPinCirclesOpt));
  mCircleGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

  // line
  mLineGraphicsItem->setLineWidth(UnsignedLength(158750));
  mLineGraphicsItem->setLayer(lp.getLayer(GraphicsLayer::sSymbolOutlines));
  mLineGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

  // text
  mTextGraphicsItem->setHeight(PositiveLength(Length::fromMm(qreal(2))));
  mTextGraphicsItem->setAlignment(Alignment(HAlign::left(), VAlign::center()));
  mTextGraphicsItem->setLayer(lp.getLayer(GraphicsLayer::sSymbolPinNames));
  mTextGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

  // pin properties
  setPosition(mPin.getPosition());
  setRotation(mPin.getRotation());
  setLength(mPin.getLength());
  setName(mPin.getName());

  // register to the pin to get attribute updates
  mPin.registerGraphicsItem(*this);
}

SymbolPinGraphicsItem::~SymbolPinGraphicsItem() noexcept {
  mPin.unregisterGraphicsItem(*this);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPinGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void SymbolPinGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void SymbolPinGraphicsItem::setLength(const UnsignedLength& length) noexcept {
  mLineGraphicsItem->setLine(Point(0, 0), Point(*length, 0));
  mTextGraphicsItem->setPosition(Point(length + Length(800000), Length(0)));
}

void SymbolPinGraphicsItem::setName(const CircuitIdentifier& name) noexcept {
  setToolTip(*name);
  mTextGraphicsItem->setText(*name);
}

void SymbolPinGraphicsItem::setSelected(bool selected) noexcept {
  mCircleGraphicsItem->setSelected(selected);
  mLineGraphicsItem->setSelected(selected);
  mTextGraphicsItem->setSelected(selected);
  QGraphicsItem::setSelected(selected);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SymbolPinGraphicsItem::shape() const noexcept {
  QPainterPath p;
  p.addEllipse(mCircleGraphicsItem->boundingRect());
  return p;
}

void SymbolPinGraphicsItem::paint(QPainter*                       painter,
                                  const QStyleOptionGraphicsItem* option,
                                  QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
