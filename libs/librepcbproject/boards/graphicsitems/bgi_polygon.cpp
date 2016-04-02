/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <QtWidgets>
#include <QPrinter>
#include "bgi_polygon.h"
#include "../items/bi_polygon.h"
#include "../board.h"
#include "../../project.h"
#include <librepcbcommon/boardlayer.h>
#include <librepcbcommon/geometry/polygon.h>
#include "../boardlayerstack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Polygon::BGI_Polygon(BI_Polygon& polygon) noexcept :
    BGI_Base(), mBiPolygon(polygon), mPolygon(polygon.getPolygon()), mLayer(nullptr)
{
    updateCacheAndRepaint();
}

BGI_Polygon::~BGI_Polygon() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_Polygon::isSelectable() const noexcept
{
    return mLayer && mLayer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Polygon::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    setZValue(Board::ZValue_Default);

    mLayer = getBoardLayer(mPolygon.getLayerId());

    // set shape and bounding rect
    mShape = mPolygon.toQPainterPathPx();
    mBoundingRect = mShape.boundingRect();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Polygon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const bool selected = mBiPolygon.isSelected();
    //const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    //const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
    Q_UNUSED(option);

    if (mLayer && mLayer->isVisible()) {
        // draw polygon outline
        painter->setPen(QPen(mLayer->getColor(selected), mPolygon.getWidth().toPx(),
                             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(mPolygon.toQPainterPathPx());
    }

#ifdef QT_DEBUG
    // draw bounding rect
    const BoardLayer* layer = mBiPolygon.getBoard().getLayerStack().getBoardLayer(
        BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects);
    if (layer) {
        if (layer->isVisible()) {
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(mBoundingRect);
        }
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_Polygon::getBoardLayer(int id) const noexcept
{
    if (mBiPolygon.getIsMirrored()) id = BoardLayer::getMirroredLayerId(id);
    return mBiPolygon.getBoard().getLayerStack().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
