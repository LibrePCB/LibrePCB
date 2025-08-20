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
#include "sgi_text.h"

#include "../../../graphics/graphicslayerlist.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/textgraphicsitem.h"
#include "../schematicgraphicsscene.h"

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

SGI_Text::SGI_Text(SI_Text& text, std::weak_ptr<SGI_Symbol> symbolItem,
                   const GraphicsLayerList& layers) noexcept
  : QGraphicsItemGroup(),
    mText(text),
    mSymbolGraphicsItem(symbolItem),
    mLayers(layers),
    mTextGraphicsItem(new TextGraphicsItem(mText.getTextObj(), layers, this)),
    mAnchorGraphicsItem(new LineGraphicsItem()),
    mOnEditedSlot(*this, &SGI_Text::textEdited),
    mOnSymbolEditedSlot(*this, &SGI_Text::symbolGraphicsItemEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_Texts);

  mAnchorGraphicsItem->setZValue(SchematicGraphicsScene::ZValue_TextAnchors);

  updateText();
  updateAnchorLayer();
  updateAnchorLine();

  mText.onEdited.attach(mOnEditedSlot);
  if (auto ptr = symbolItem.lock()) {
    ptr->onEdited.attach(mOnSymbolEditedSlot);
  }
}

SGI_Text::~SGI_Text() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SGI_Text::shape() const noexcept {
  Q_ASSERT(mTextGraphicsItem);
  return mTextGraphicsItem->mapToParent(mTextGraphicsItem->shape());
}

QVariant SGI_Text::itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mTextGraphicsItem &&
      mAnchorGraphicsItem) {
    mTextGraphicsItem->setSelected(value.toBool());
    mAnchorGraphicsItem->setSelected(value.toBool());
    updateAnchorLayer();
  } else if ((change == ItemSceneHasChanged) && mAnchorGraphicsItem) {
    if (QGraphicsScene* s = mAnchorGraphicsItem->scene()) {
      s->removeItem(mAnchorGraphicsItem.data());
    }
    if (QGraphicsScene* s = scene()) {
      s->addItem(mAnchorGraphicsItem.data());
    }
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_Text::textEdited(const SI_Text& obj, SI_Text::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_Text::Event::PositionChanged:
      updateAnchorLine();
      break;
    case SI_Text::Event::LayerNameChanged:
      updateAnchorLayer();
      break;
    case SI_Text::Event::TextChanged:
      updateText();
      break;
    case SI_Text::Event::LockedChanged: {
      updateAnchorLayer();
      break;
    }
    default:
      qWarning() << "Unhandled switch-case in SGI_Text::textEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_Text::symbolGraphicsItemEdited(const SGI_Symbol& obj,
                                        SGI_Symbol::Event event) noexcept {
  switch (event) {
    case SGI_Symbol::Event::PositionChanged: {
      updateAnchorLine();
      break;
    }
    case SGI_Symbol::Event::SelectionChanged: {
      setSelected(obj.isSelected());
      break;
    }
    default: {
      break;
    }
  }
}

void SGI_Text::updateText() noexcept {
  Q_ASSERT(mTextGraphicsItem);
  mTextGraphicsItem->setTextOverride(mText.getText());
}

void SGI_Text::updateAnchorLayer() noexcept {
  Q_ASSERT(mAnchorGraphicsItem);
  if (mText.getSymbol() && (!mText.getTextObj().isLocked()) && isSelected()) {
    mAnchorGraphicsItem->setLayer(mLayers.get(mText.getTextObj().getLayer()));
  } else {
    mAnchorGraphicsItem->setLayer(nullptr);
  }

  mTextGraphicsItem->setOriginCrossVisible(!mText.getTextObj().isLocked());
}

void SGI_Text::updateAnchorLine() noexcept {
  Q_ASSERT(mAnchorGraphicsItem);
  if (SI_Symbol* symbol = mText.getSymbol()) {
    mAnchorGraphicsItem->setLine(mText.getTextObj().getPosition(),
                                 symbol->getPosition());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
