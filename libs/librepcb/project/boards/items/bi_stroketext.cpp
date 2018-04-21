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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_stroketext.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
#include "./bi_footprint.h"
#include <librepcb/common/font/strokefontpool.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/linegraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/geometry/stroketext.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_StrokeText::BI_StrokeText(Board& board, const BI_StrokeText& other) :
    BI_Base(board), mFootprint(nullptr)
{
    mText.reset(new StrokeText(Uuid::createRandom(), *other.mText));
    init();
}

BI_StrokeText::BI_StrokeText(Board& board, const SExpression& node) :
    BI_Base(board), mFootprint(nullptr)
{
    mText.reset(new StrokeText(node));
    init();

}

BI_StrokeText::BI_StrokeText(Board& board, const StrokeText& text) :
    BI_Base(board), mFootprint(nullptr)
{
    mText.reset(new StrokeText(text));
    init();
}

void BI_StrokeText::init()
{
    mText->registerObserver(*this);
    mText->setAttributeProvider(&mBoard);
    mText->setFont(&getProject().getStrokeFonts().getFont(mBoard.getDefaultFontName())); // can throw

    mGraphicsItem.reset(new StrokeTextGraphicsItem(*mText, mBoard.getLayerStack()));
    mAnchorGraphicsItem.reset(new LineGraphicsItem());
    updateGraphicsItems();

    // connect to the "attributes changed" signal of the board
    connect(&mBoard, &Board::attributesChanged, this, &BI_StrokeText::boardAttributesChanged);
}

BI_StrokeText::~BI_StrokeText() noexcept
{
    mAnchorGraphicsItem.reset();
    mGraphicsItem.reset();
    mText->unregisterObserver(*this);
    mText.reset();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_StrokeText::setFootprint(BI_Footprint* footprint) noexcept
{
    mFootprint = footprint;
    updateGraphicsItems();
}

void BI_StrokeText::updateGraphicsItems() noexcept
{
    // update z-value
    Board::ItemZValue zValue = Board::ZValue_Texts;
    if (GraphicsLayer::isTopLayer(mText->getLayerName())) {
        zValue = Board::ZValue_TextsTop;
    } else if (GraphicsLayer::isBottomLayer(mText->getLayerName())) {
        zValue = Board::ZValue_TextsBottom;
    }
    mGraphicsItem->setZValue(static_cast<qreal>(zValue));
    mAnchorGraphicsItem->setZValue(static_cast<qreal>(zValue));

    // show anchor line only if there is a footprint and the text is selected
    if (mFootprint && isSelected()) {
        mAnchorGraphicsItem->setLine(mText->getPosition(), mFootprint->getPosition());
        mAnchorGraphicsItem->setLayer(mBoard.getLayerStack().getLayer(mText->getLayerName()));
    } else {
        mAnchorGraphicsItem->setLayer(nullptr);
    }
}

void BI_StrokeText::addToBoard()
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::addToBoard(mGraphicsItem.data());
    mBoard.getGraphicsScene().addItem(*mAnchorGraphicsItem);
}

void BI_StrokeText::removeFromBoard()
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::removeFromBoard(mGraphicsItem.data());
    mBoard.getGraphicsScene().removeItem(*mAnchorGraphicsItem);
}

void BI_StrokeText::serialize(SExpression& root) const
{
    mText->serialize(root);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

const Point& BI_StrokeText::getPosition() const noexcept
{
    return mText->getPosition();
}

QPainterPath BI_StrokeText::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

const Uuid& BI_StrokeText::getUuid() const noexcept
{
    return mText->getUuid();
}

bool BI_StrokeText::isSelectable() const noexcept
{
    const GraphicsLayer* layer = mBoard.getLayerStack().getLayer(mText->getLayerName());
    return layer && layer->isVisible();
}

void BI_StrokeText::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->setSelected(selected);
    updateGraphicsItems();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_StrokeText::boardAttributesChanged()
{
    mText->updatePaths();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
