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
#include "../../project.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/geometry/polygon.h>
#include "../graphicsitems/bgi_polygon.h"

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

BI_Polygon::BI_Polygon(Board& board, const DomElement& domElement) :
    BI_Base(board)
{
    mPolygon.reset(new Polygon(domElement));
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
    mGraphicsItem.reset(new BGI_Polygon(*this));
    mGraphicsItem->setPos(getPosition().toPxQPointF());
    mGraphicsItem->setRotation(Angle::deg0().toDeg());

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

void BI_Polygon::addToBoard(GraphicsScene& scene)
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::addToBoard(scene, *mGraphicsItem);
}

void BI_Polygon::removeFromBoard(GraphicsScene& scene)
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    BI_Base::removeFromBoard(scene, *mGraphicsItem);
}

void BI_Polygon::serialize(DomElement& root) const
{
    mPolygon->serialize(root);
}

bool BI_Polygon::getAttributeValue(const QString& attrKey, bool passToParents, QString& value) const noexcept
{
    // no local attributes available
    if (passToParents)
        return mBoard.getAttributeValue(attrKey, true, value);
    else
        return false;
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
    return mGraphicsItem->isSelectable();
}

void BI_Polygon::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_Polygon::boardAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
