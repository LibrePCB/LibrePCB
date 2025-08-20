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

#include "graphicslayer.h"
#include "graphicslayerlist.h"
#include "origincrossgraphicsitem.h"
#include "primitivetextgraphicsitem.h"

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

TextGraphicsItem::TextGraphicsItem(Text& text, const GraphicsLayerList& layers,
                                   QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mText(text),
    mLayers(layers),
    mTextOverride(std::nullopt),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mOnEditedSlot(*this, &TextGraphicsItem::textEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  setPos(mText.getPosition().toPxQPointF());

  mTextGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mTextGraphicsItem->setHeight(mText.getHeight());
  mTextGraphicsItem->setLayer(mLayers.get(mText.getLayer()));
  mTextGraphicsItem->setRotation(mText.getRotation());
  mTextGraphicsItem->setAlignment(mText.getAlign());
  updateText();

  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));
  mOriginCrossGraphicsItem->setLayer(
      mLayers.get(Theme::Color::sSchematicReferences));
  mOriginCrossGraphicsItem->setRotation(mText.getRotation());

  // register to the text to get attribute updates
  mText.onEdited.attach(mOnEditedSlot);
}

TextGraphicsItem::~TextGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void TextGraphicsItem::setOriginCrossVisible(bool visible) noexcept {
  mOriginCrossGraphicsItem->setLayer(
      visible ? mLayers.get(Theme::Color::sSchematicReferences) : nullptr);
}

void TextGraphicsItem::setTextOverride(
    const std::optional<QString>& text) noexcept {
  if (text != mTextOverride) {
    mTextOverride = text;
    updateText();
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath TextGraphicsItem::shape() const noexcept {
  Q_ASSERT(mTextGraphicsItem && mOriginCrossGraphicsItem);
  return mTextGraphicsItem->mapToParent(mTextGraphicsItem->shape()) |
      mOriginCrossGraphicsItem->shape();
}

QVariant TextGraphicsItem::itemChange(GraphicsItemChange change,
                                      const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mTextGraphicsItem &&
      mOriginCrossGraphicsItem) {
    mTextGraphicsItem->setSelected(value.toBool());
    mOriginCrossGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void TextGraphicsItem::textEdited(const Text& text,
                                  Text::Event event) noexcept {
  switch (event) {
    case Text::Event::LayerChanged:
      mTextGraphicsItem->setLayer(mLayers.get(text.getLayer()));
      break;
    case Text::Event::TextChanged:
      updateText();
      break;
    case Text::Event::PositionChanged:
      setPos(text.getPosition().toPxQPointF());
      break;
    case Text::Event::RotationChanged:
      mTextGraphicsItem->setRotation(text.getRotation());
      mOriginCrossGraphicsItem->setRotation(text.getRotation());
      break;
    case Text::Event::HeightChanged:
      mTextGraphicsItem->setHeight(text.getHeight());
      break;
    case Text::Event::AlignChanged:
      mTextGraphicsItem->setAlignment(text.getAlign());
      break;
    default:
      qWarning() << "Unhandled switch-case in TextGraphicsItem::textEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void TextGraphicsItem::updateText() noexcept {
  mTextGraphicsItem->setText(mTextOverride ? (*mTextOverride)
                                           : mText.getText());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
