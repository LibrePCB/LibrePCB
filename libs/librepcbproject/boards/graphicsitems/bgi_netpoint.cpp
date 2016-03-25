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
#include "bgi_netpoint.h"
#include "../items/bi_netpoint.h"
#include "../board.h"
#include "../../project.h"
#include "../boardlayerstack.h"
#include "../../circuit/netsignal.h"
#include <librepcbcommon/boardlayer.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_NetPoint::BGI_NetPoint(BI_NetPoint& netpoint) noexcept :
    BGI_Base(), mNetPoint(netpoint)
{
    updateCacheAndRepaint();
}

BGI_NetPoint::~BGI_NetPoint() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_NetPoint::isSelectable() const noexcept
{
    return mNetPoint.getLayer().isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_NetPoint::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    // set Z value
    setZValue(getZValueOfCopperLayer(mNetPoint.getLayer().getId()));

    qreal radius = mNetPoint.getMaxLineWidth().toPx() / 2;
    mBoundingRect = QRectF(-radius, -radius, 2*radius, 2*radius);

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_NetPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    bool highlight = mNetPoint.isSelected() || mNetPoint.getNetSignal().isHighlighted();

#ifdef QT_DEBUG
    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
#else
    Q_UNUSED(highlight);
    Q_UNUSED(painter);
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_NetPoint::getBoardLayer(int id) const noexcept
{
    return mNetPoint.getBoard().getLayerStack().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
