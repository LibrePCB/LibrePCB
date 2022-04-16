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
#include "symbolpingraphicsitem.h"

#include "../../graphics/graphicslayer.h"
#include "../../graphics/linegraphicsitem.h"
#include "../../graphics/primitivecirclegraphicsitem.h"
#include "../../graphics/primitivetextgraphicsitem.h"
#include "../../types/angle.h"
#include "../../types/point.h"
#include "../../utils/toolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolPinGraphicsItem::SymbolPinGraphicsItem(std::shared_ptr<SymbolPin> pin,
                                             const IF_GraphicsLayerProvider& lp,
                                             QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mPin(pin),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnEditedSlot(*this, &SymbolPinGraphicsItem::pinEdited) {
  Q_ASSERT(mPin);

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
  mTextGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mTextGraphicsItem->setHeight(SymbolPin::getNameHeight());
  mTextGraphicsItem->setLayer(lp.getLayer(GraphicsLayer::sSymbolPinNames));
  mTextGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  updateTextRotationAndAlignment();

  // pin properties
  setPosition(mPin->getPosition());
  setRotation(mPin->getRotation());
  setLength(mPin->getLength());
  setName(mPin->getName());

  // Register to the pin to get notified about any modifications.
  mPin->onEdited.attach(mOnEditedSlot);
}

SymbolPinGraphicsItem::~SymbolPinGraphicsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPinGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void SymbolPinGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
  updateTextRotationAndAlignment();  // Auto-rotation may need to be updated.
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

void SymbolPinGraphicsItem::paint(QPainter* painter,
                                  const QStyleOptionGraphicsItem* option,
                                  QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolPinGraphicsItem::pinEdited(const SymbolPin& pin,
                                      SymbolPin::Event event) noexcept {
  switch (event) {
    case SymbolPin::Event::UuidChanged:
      break;
    case SymbolPin::Event::NameChanged:
      setName(pin.getName());
      break;
    case SymbolPin::Event::PositionChanged:
      setPosition(pin.getPosition());
      break;
    case SymbolPin::Event::LengthChanged:
      setLength(pin.getLength());
      break;
    case SymbolPin::Event::RotationChanged:
      setRotation(pin.getRotation());
      break;
    default:
      qWarning()
          << "Unhandled switch-case in SymbolPinGraphicsItem::pinEdited()";
      break;
  }
}

void SymbolPinGraphicsItem::setLength(const UnsignedLength& length) noexcept {
  mLineGraphicsItem->setLine(Point(0, 0), Point(*length, 0));
  mTextGraphicsItem->setPosition(mPin->getNamePosition());
}

void SymbolPinGraphicsItem::setName(const CircuitIdentifier& name) noexcept {
  setToolTip(*name);
  mTextGraphicsItem->setText(*name);
}

void SymbolPinGraphicsItem::updateTextRotationAndAlignment() noexcept {
  Angle rotation(0);
  Alignment alignment(HAlign::left(), VAlign::center());
  if (Toolbox::isTextUpsideDown(mPin->getRotation(), false)) {
    rotation += Angle::deg180();
    alignment.mirror();
  }
  mTextGraphicsItem->setRotation(rotation);
  mTextGraphicsItem->setAlignment(alignment);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
