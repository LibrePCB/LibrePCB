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
#include "textgraphicsitem.h"

#include "../attribute/attributesubstitutor.h"
#include "../graphics/graphicslayer.h"
#include "../utils/toolbox.h"
#include "origincrossgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TextGraphicsItem::TextGraphicsItem(Text& text,
                                   const IF_GraphicsLayerProvider& lp,
                                   QGraphicsItem* parent) noexcept
  : PrimitiveTextGraphicsItem(parent),
    mText(text),
    mLayerProvider(lp),
    mAttributeProvider(nullptr),
    mOnEditedSlot(*this, &TextGraphicsItem::textEdited) {
  setFont(TextGraphicsItem::Font::SansSerif);
  setPosition(mText.getPosition());
  setHeight(mText.getHeight());
  setLayer(mLayerProvider.getLayer(*mText.getLayerName()));
  setRotationAndAlignment(mText.getRotation(), mText.getAlign());
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);
  updateText();

  // add origin cross
  mOriginCrossGraphicsItem.reset(new OriginCrossGraphicsItem(this));
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));
  mOriginCrossGraphicsItem->setLayer(
      mLayerProvider.getLayer(GraphicsLayer::sSchematicReferences));  // TODO

  // register to the text to get attribute updates
  mText.onEdited.attach(mOnEditedSlot);
}

TextGraphicsItem::~TextGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void TextGraphicsItem::setRotation(const Angle& rot) noexcept {
  setRotationAndAlignment(rot, mText.getAlign());
}

void TextGraphicsItem::setAlignment(const Alignment& align) noexcept {
  setRotationAndAlignment(mText.getRotation(), align);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void TextGraphicsItem::setAttributeProvider(
    const AttributeProvider* provider) noexcept {
  if (provider != mAttributeProvider) {
    mAttributeProvider = provider;
    updateText();
  }
}

void TextGraphicsItem::updateText() noexcept {
  QString text = mText.getText();
  if (mAttributeProvider) {
    text = AttributeSubstitutor::substitute(text, mAttributeProvider);
  }
  setText(text);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void TextGraphicsItem::textEdited(const Text& text,
                                  Text::Event event) noexcept {
  switch (event) {
    case Text::Event::LayerNameChanged:
      setLayer(mLayerProvider.getLayer(*text.getLayerName()));
      break;
    case Text::Event::TextChanged:
      updateText();
      break;
    case Text::Event::PositionChanged:
      setPosition(text.getPosition());
      break;
    case Text::Event::RotationChanged:
      setRotationAndAlignment(text.getRotation(), text.getAlign());
      break;
    case Text::Event::HeightChanged:
      setHeight(text.getHeight());
      break;
    case Text::Event::AlignChanged:
      setRotationAndAlignment(text.getRotation(), text.getAlign());
      break;
    default:
      qWarning() << "Unhandled switch-case in TextGraphicsItem::textEdited()";
      break;
  }
}

void TextGraphicsItem::setRotationAndAlignment(Angle rotation,
                                               Alignment align) noexcept {
  if (Toolbox::isTextUpsideDown(rotation, false)) {
    rotation += Angle::deg180();
    align.mirror();
  }
  PrimitiveTextGraphicsItem::setRotation(rotation);
  PrimitiveTextGraphicsItem::setAlignment(align);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
