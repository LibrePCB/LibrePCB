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
#include "bi_stroketext.h"

#include "../../../font/strokefontpool.h"
#include "../../../geometry/stroketext.h"
#include "../../../graphics/graphicsscene.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/stroketextgraphicsitem.h"
#include "../../project.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "./bi_footprint.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_StrokeText::BI_StrokeText(Board& board, const BI_StrokeText& other)
  : BI_Base(board),
    mFootprint(nullptr),
    mOnStrokeTextEditedSlot(*this, &BI_StrokeText::strokeTextEdited) {
  mText.reset(new StrokeText(Uuid::createRandom(), *other.mText));
  init();
}

BI_StrokeText::BI_StrokeText(Board& board, const SExpression& node,
                             const Version& fileFormat)
  : BI_Base(board),
    mFootprint(nullptr),
    mOnStrokeTextEditedSlot(*this, &BI_StrokeText::strokeTextEdited) {
  mText.reset(new StrokeText(node, fileFormat));
  init();
}

BI_StrokeText::BI_StrokeText(Board& board, const StrokeText& text)
  : BI_Base(board),
    mFootprint(nullptr),
    mOnStrokeTextEditedSlot(*this, &BI_StrokeText::strokeTextEdited) {
  mText.reset(new StrokeText(text));
  init();
}

void BI_StrokeText::init() {
  mText->onEdited.attach(mOnStrokeTextEditedSlot);
  mText->setAttributeProvider(&mBoard);
  mText->setFont(&getProject().getStrokeFonts().getFont(
      mBoard.getDefaultFontName()));  // can throw

  mGraphicsItem.reset(
      new StrokeTextGraphicsItem(*mText, mBoard.getLayerStack()));
  mAnchorGraphicsItem.reset(new LineGraphicsItem());
  updateGraphicsItems();

  // connect to the "attributes changed" signal of the board
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_StrokeText::boardOrFootprintAttributesChanged);
}

BI_StrokeText::~BI_StrokeText() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_StrokeText::setFootprint(BI_Footprint* footprint) noexcept {
  if (mFootprint) {
    disconnect(mFootprint, &BI_Footprint::attributesChanged, this,
               &BI_StrokeText::boardOrFootprintAttributesChanged);
  }

  mFootprint = footprint;
  updateGraphicsItems();

  // Text might need to be updated if footprint attributes have changed.
  if (mFootprint) {
    connect(mFootprint, &BI_Footprint::attributesChanged, this,
            &BI_StrokeText::boardOrFootprintAttributesChanged);
  }
}

void BI_StrokeText::updateGraphicsItems() noexcept {
  // update z-value
  Board::ItemZValue zValue = Board::ZValue_Texts;
  if (GraphicsLayer::isTopLayer(*mText->getLayerName())) {
    zValue = Board::ZValue_TextsTop;
  } else if (GraphicsLayer::isBottomLayer(*mText->getLayerName())) {
    zValue = Board::ZValue_TextsBottom;
  }
  mGraphicsItem->setZValue(static_cast<qreal>(zValue));
  mAnchorGraphicsItem->setZValue(static_cast<qreal>(zValue));

  // show anchor line only if there is a footprint and the text is selected
  if (mFootprint && isSelected()) {
    mAnchorGraphicsItem->setLine(mText->getPosition(),
                                 mFootprint->getPosition());
    mAnchorGraphicsItem->setLayer(
        mBoard.getLayerStack().getLayer(*mText->getLayerName()));
  } else {
    mAnchorGraphicsItem->setLayer(nullptr);
  }
}

void BI_StrokeText::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard(mGraphicsItem.data());
  mBoard.getGraphicsScene().addItem(*mAnchorGraphicsItem);
}

void BI_StrokeText::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard(mGraphicsItem.data());
  mBoard.getGraphicsScene().removeItem(*mAnchorGraphicsItem);
}

void BI_StrokeText::serialize(SExpression& root) const {
  mText->serialize(root);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

const Point& BI_StrokeText::getPosition() const noexcept {
  return mText->getPosition();
}

QPainterPath BI_StrokeText::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

const Uuid& BI_StrokeText::getUuid() const noexcept {
  return mText->getUuid();
}

bool BI_StrokeText::isSelectable() const noexcept {
  const GraphicsLayer* layer =
      mBoard.getLayerStack().getLayer(*mText->getLayerName());
  return layer && layer->isVisible();
}

void BI_StrokeText::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->setSelected(selected);
  updateGraphicsItems();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BI_StrokeText::boardOrFootprintAttributesChanged() {
  mText->updatePaths();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_StrokeText::strokeTextEdited(const StrokeText& text,
                                     StrokeText::Event event) noexcept {
  Q_UNUSED(text);
  switch (event) {
    case StrokeText::Event::LayerNameChanged:
    case StrokeText::Event::PositionChanged:
      updateGraphicsItems();
      break;
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
