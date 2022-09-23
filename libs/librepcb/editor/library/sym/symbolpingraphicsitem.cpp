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

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/graphics/linegraphicsitem.h>
#include <librepcb/core/graphics/primitivecirclegraphicsitem.h>
#include <librepcb/core/graphics/primitivetextgraphicsitem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>
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

SymbolPinGraphicsItem::SymbolPinGraphicsItem(
    std::shared_ptr<SymbolPin> pin, const IF_GraphicsLayerProvider& lp,
    std::shared_ptr<const Component> cmp,
    std::shared_ptr<const ComponentSymbolVariantItem> cmpItem,
    QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mPin(pin),
    mLayerProvider(lp),
    mComponent(cmp),
    mItem(cmpItem),
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
  updateText();

  // pin properties
  setPosition(mPin->getPosition());
  setRotation(mPin->getRotation());
  setLength(mPin->getLength());

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
 *  General Methods
 ******************************************************************************/

void SymbolPinGraphicsItem::updateText() noexcept {
  QString text;
  if (mItem) {
    if (auto i = mItem->getPinSignalMap().find(mPin->getUuid())) {
      auto signal = (mComponent && i->getSignalUuid())
          ? mComponent->getSignals().find(*i->getSignalUuid())
          : nullptr;
      if (signal && signal->isRequired()) {
        mCircleGraphicsItem->setLineLayer(
            mLayerProvider.getLayer(GraphicsLayer::sSymbolPinCirclesReq));
      } else if (signal && (!signal->isRequired())) {
        mCircleGraphicsItem->setLineLayer(
            mLayerProvider.getLayer(GraphicsLayer::sSymbolPinCirclesOpt));
      } else {
        mCircleGraphicsItem->setLineLayer(nullptr);
      }
      if (i->getDisplayType() == CmpSigPinDisplayType::none()) {
        // Nothing to do, leave text empty.
      } else if (i->getDisplayType() == CmpSigPinDisplayType::pinName()) {
        text = *mPin->getName();
      } else if (i->getDisplayType() ==
                 CmpSigPinDisplayType::componentSignal()) {
        if (signal) {
          text = *signal->getName();
        }
      } else if (i->getDisplayType() == CmpSigPinDisplayType::netSignal()) {
        if (signal && (!signal->getForcedNetName().isEmpty())) {
          text = signal->getForcedNetName();
        } else {
          text = "(NET)";
        }
      } else {
        qCritical() << "SymbolPinGraphicsItem: Unknown pin display type!";
      }
    } else {
      qCritical() << "SymbolPinGraphicsItem: Pin not found in pin-signal map!";
    }
  } else {
    text = *mPin->getName();
  }
  setToolTip(text);
  mTextGraphicsItem->setText(text);
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
      updateText();
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

}  // namespace editor
}  // namespace librepcb
