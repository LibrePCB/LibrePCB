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
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/linegraphicsitem.h"
#include "../../graphics/primitivecirclegraphicsitem.h"
#include "../../graphics/primitivetextgraphicsitem.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/utils/overlinemarkupparser.h>
#include <librepcb/core/workspace/theme.h>

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
    std::shared_ptr<SymbolPin> pin, const GraphicsLayerList& layers,
    QPointer<const Component> cmp,
    std::shared_ptr<const ComponentSymbolVariantItem> cmpItem,
    bool hideIfUnused, QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mPin(pin),
    mLayers(layers),
    mComponent(cmp),
    mItem(cmpItem),
    mHideIfUnused(hideIfUnused),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mNameGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mNumbersGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnEditedSlot(*this, &SymbolPinGraphicsItem::pinEdited) {
  Q_ASSERT(mPin);

  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(10);

  // circle
  mCircleGraphicsItem->setDiameter(UnsignedLength(1200000));
  mCircleGraphicsItem->setLineLayer(
      layers.get(Theme::Color::sSchematicOptionalPins));
  mCircleGraphicsItem->setShapeMode(
      PrimitiveCircleGraphicsItem::ShapeMode::FilledOutline);

  // line
  mLineGraphicsItem->setRotation(mPin->getRotation());
  mLineGraphicsItem->setLineWidth(UnsignedLength(158750));
  mLineGraphicsItem->setLayer(layers.get(Theme::Color::sSchematicPinLines));

  // name
  mNameGraphicsItem->setRotation(mPin->getRotation() + mPin->getNameRotation());
  mNameGraphicsItem->setAlignment(mPin->getNameAlignment());
  mNameGraphicsItem->setHeight(mPin->getNameHeight());
  mNameGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mNameGraphicsItem->setLayer(layers.get(Theme::Color::sSchematicPinNames));
  updateNamePosition();
  updateText();

  // numbers
  mNumbersGraphicsItem->setRotation(mPin->getRotation());
  mNumbersGraphicsItem->setHeight(SymbolPin::getNumbersHeight());
  mNumbersGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mNumbersGraphicsItem->setLayer(
      layers.get(Theme::Color::sSchematicPinNumbers));
  mNumbersGraphicsItem->setOpacity(0.4);
  mNumbersGraphicsItem->setText("1…");
  updateNumbersTransform();

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
  bool isConnected = false;
  if (mItem) {
    if (auto i = mItem->getPinSignalMap().find(mPin->getUuid())) {
      isConnected = i->getSignalUuid().has_value();
      auto signal = (mComponent && i->getSignalUuid())
          ? mComponent->getSignals().find(*i->getSignalUuid())
          : nullptr;
      if (signal && signal->isRequired()) {
        mCircleGraphicsItem->setLineLayer(
            mLayers.get(Theme::Color::sSchematicRequiredPins));
      } else if (signal && (!signal->isRequired())) {
        mCircleGraphicsItem->setLineLayer(
            mLayers.get(Theme::Color::sSchematicOptionalPins));
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
  mNameGraphicsItem->setText(text, true);

  // Also update the pin's visibility, despite the misleading method name o_o
  setVisible(isConnected || (!mItem) || (!mHideIfUnused));
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
      mLineGraphicsItem && mNameGraphicsItem && mNumbersGraphicsItem) {
    mCircleGraphicsItem->setSelected(value.toBool());
    mLineGraphicsItem->setSelected(value.toBool());
    mNameGraphicsItem->setSelected(value.toBool());
    mNumbersGraphicsItem->setSelected(value.toBool());
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
      updateNumbersTransform();
      break;
    case SymbolPin::Event::RotationChanged:
      mLineGraphicsItem->setRotation(pin.getRotation());
      mNameGraphicsItem->setRotation(pin.getRotation() + pin.getNameRotation());
      updateNamePosition();
      mNumbersGraphicsItem->setRotation(pin.getRotation());
      updateNumbersTransform();
      break;
    case SymbolPin::Event::NamePositionChanged:
      updateNamePosition();
      break;
    case SymbolPin::Event::NameHeightChanged:
      mNameGraphicsItem->setHeight(pin.getNameHeight());
      break;
    case SymbolPin::Event::NameRotationChanged:
      mNameGraphicsItem->setRotation(pin.getRotation() + pin.getNameRotation());
      break;
    case SymbolPin::Event::NameAlignmentChanged:
      mNameGraphicsItem->setAlignment(pin.getNameAlignment());
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

void SymbolPinGraphicsItem::updateNamePosition() noexcept {
  mNameGraphicsItem->setPosition(
      mPin->getNamePosition().rotated(mPin->getRotation()));
}

void SymbolPinGraphicsItem::updateNumbersTransform() noexcept {
  const bool flipped = Toolbox::isTextUpsideDown(mPin->getRotation());
  mNumbersGraphicsItem->setPosition(
      mPin->getNumbersPosition(flipped).rotated(mPin->getRotation()));
  mNumbersGraphicsItem->setAlignment(SymbolPin::getNumbersAlignment(flipped));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
