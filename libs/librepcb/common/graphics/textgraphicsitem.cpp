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
#include "textgraphicsitem.h"

#include "../graphics/graphicslayer.h"
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

TextGraphicsItem::TextGraphicsItem(Text&                           text,
                                   const IF_GraphicsLayerProvider& lp,
                                   QGraphicsItem* parent) noexcept
  : PrimitiveTextGraphicsItem(parent), mText(text), mLayerProvider(lp) {
  setFont(TextGraphicsItem::Font::SansSerif);
  setPosition(mText.getPosition());
  setRotation(mText.getRotation());
  setText(mText.getText());
  setHeight(mText.getHeight());
  setAlignment(mText.getAlign());
  setLayer(mLayerProvider.getLayer(*mText.getLayerName()));
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  // add origin cross
  mOriginCrossGraphicsItem.reset(new OriginCrossGraphicsItem(this));
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));
  mOriginCrossGraphicsItem->setLayer(
      mLayerProvider.getLayer(GraphicsLayer::sSchematicReferences));  // TODO

  // register to the text to get attribute updates
  mText.registerObserver(*this);
}

TextGraphicsItem::~TextGraphicsItem() noexcept {
  mText.unregisterObserver(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void TextGraphicsItem::textLayerNameChanged(
    const GraphicsLayerName& newLayerName) noexcept {
  setLayer(mLayerProvider.getLayer(*newLayerName));
}

void TextGraphicsItem::textTextChanged(const QString& newText) noexcept {
  setText(newText);
}

void TextGraphicsItem::textPositionChanged(const Point& newPos) noexcept {
  setPosition(newPos);
}

void TextGraphicsItem::textRotationChanged(const Angle& newRot) noexcept {
  setRotation(newRot);
}

void TextGraphicsItem::textHeightChanged(
    const PositiveLength& newHeight) noexcept {
  setHeight(newHeight);
}

void TextGraphicsItem::textAlignChanged(const Alignment& newAlign) noexcept {
  setAlignment(newAlign);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
