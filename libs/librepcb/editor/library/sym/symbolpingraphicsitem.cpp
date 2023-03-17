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

#include "../../graphics/linegraphicsitem.h"
#include "../../graphics/primitivecirclegraphicsitem.h"
#include "../../graphics/primitivetextgraphicsitem.h"

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

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
  : QGraphicsItemGroup(parent),
    mPin(pin),
    mLayerProvider(lp),
    mComponent(cmp),
    mItem(cmpItem),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnEditedSlot(*this, &SymbolPinGraphicsItem::pinEdited) {
  Q_ASSERT(mPin);

  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(10);

  // circle
  mCircleGraphicsItem->setDiameter(UnsignedLength(1200000));
  mCircleGraphicsItem->setLineLayer(
      lp.getLayer(GraphicsLayer::sSymbolPinCirclesOpt));
  mCircleGraphicsItem->setShapeMode(
      PrimitiveCircleGraphicsItem::ShapeMode::FilledOutline);

  // line
  mLineGraphicsItem->setRotation(mPin->getRotation());
  mLineGraphicsItem->setLineWidth(UnsignedLength(158750));
  mLineGraphicsItem->setLayer(lp.getLayer(GraphicsLayer::sSymbolPinLines));

  // text
  mTextGraphicsItem->setRotation(mPin->getRotation() + mPin->getNameRotation());
  mTextGraphicsItem->setAlignment(mPin->getNameAlignment());
  mTextGraphicsItem->setHeight(mPin->getNameHeight());
  mTextGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mTextGraphicsItem->setLayer(lp.getLayer(GraphicsLayer::sSymbolPinNames));
  updateTextPosition();
  updateText();

  // pin properties
  setPos(mPin->getPosition().toPxQPointF());
  setLength(mPin->getLength());

  // Register to the pin to get notified about any modifications.
  mPin->onEdited.attach(mOnEditedSlot);
}

SymbolPinGraphicsItem::~SymbolPinGraphicsItem() noexcept {
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
        qCritical() << "Unknown pin display type for pin graphics item!";
      }
    } else {
      qCritical() << "Pin not found in pin-signal map for pin graphics item!";
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
  Q_ASSERT(mCircleGraphicsItem);
  return mCircleGraphicsItem->shape();
}

QVariant SymbolPinGraphicsItem::itemChange(GraphicsItemChange change,
                                           const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mCircleGraphicsItem &&
      mLineGraphicsItem && mTextGraphicsItem) {
    mCircleGraphicsItem->setSelected(value.toBool());
    mLineGraphicsItem->setSelected(value.toBool());
    mTextGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
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
      setPos(mPin->getPosition().toPxQPointF());
      break;
    case SymbolPin::Event::LengthChanged:
      setLength(pin.getLength());
      break;
    case SymbolPin::Event::RotationChanged:
      mLineGraphicsItem->setRotation(pin.getRotation());
      mTextGraphicsItem->setRotation(pin.getRotation() + pin.getNameRotation());
      updateTextPosition();
      break;
    case SymbolPin::Event::NamePositionChanged:
      updateTextPosition();
      break;
    case SymbolPin::Event::NameHeightChanged:
      mTextGraphicsItem->setHeight(pin.getNameHeight());
      break;
    case SymbolPin::Event::NameRotationChanged:
      mTextGraphicsItem->setRotation(pin.getRotation() + pin.getNameRotation());
      break;
    case SymbolPin::Event::NameAlignmentChanged:
      mTextGraphicsItem->setAlignment(pin.getNameAlignment());
      break;
    default:
      qWarning()
          << "Unhandled switch-case in SymbolPinGraphicsItem::pinEdited():"
          << static_cast<int>(event);
      break;
  }
}

void SymbolPinGraphicsItem::setLength(const UnsignedLength& length) noexcept {
  mLineGraphicsItem->setLine(Point(0, 0), Point(*length, 0));
}

void SymbolPinGraphicsItem::updateTextPosition() noexcept {
  mTextGraphicsItem->setPosition(
      mPin->getNamePosition().rotated(mPin->getRotation()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
