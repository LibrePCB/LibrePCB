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
#include "bi_polygon.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/geometry/polygon.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Polygon::BI_Polygon(Board& board, const BI_Polygon& other) :
    BI_Base(board)
{
    mPolygon.reset(new Polygon(*other.mPolygon));
    init();
}

BI_Polygon::BI_Polygon(Board& board, const SExpression& node) :
    BI_Base(board)
{
    mPolygon.reset(new Polygon(node));
    init();
}

BI_Polygon::BI_Polygon(Board& board, const Polygon& polygon) :
    BI_Base(board)
{
    mPolygon.reset(new Polygon(polygon));
    init();
}

BI_Polygon::BI_Polygon(Board& board, const QString& layerName, const Length& lineWidth, bool fill,
                       bool isGrabArea, const Point& startPos) :
    BI_Base(board)
{
    mPolygon.reset(new Polygon(layerName, lineWidth, fill, isGrabArea, startPos));
    init();
}

void BI_Polygon::init()
{
    mGraphicsItem.reset(new PolygonGraphicsItem(*mPolygon, mBoard.getLayerStack()));
    mGraphicsItem->setZValue(Board::ZValue_Default);

    // connect to the "attributes changed" signal of the board
    connect(&mBoard, &Board::attributesChanged, this, &BI_Polygon::boardAttributesChanged);
}

BI_Polygon::~BI_Polygon() noexcept
{
    mGraphicsItem.reset();
    mPolygon.reset();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Polygon::addToBoard()
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::addToBoard(mGraphicsItem.data());
}

void BI_Polygon::removeFromBoard()
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::removeFromBoard(mGraphicsItem.data());
}

void BI_Polygon::serialize(SExpression& root) const
{
    mPolygon->serialize(root);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_Polygon::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_Polygon::isSelectable() const noexcept
{
    const GraphicsLayer* layer = mBoard.getLayerStack().getLayer(mPolygon->getLayerName());
    return layer && layer->isVisible();
}

void BI_Polygon::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->setSelected(selected);
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_Polygon::boardAttributesChanged()
{
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
