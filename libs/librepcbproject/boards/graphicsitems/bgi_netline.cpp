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
#include "bgi_netline.h"
#include "../items/bi_netline.h"
#include "../items/bi_netpoint.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
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

BGI_NetLine::BGI_NetLine(BI_NetLine& netline) noexcept :
    BGI_Base(), mNetLine(netline), mLayer(nullptr)
{
    updateCacheAndRepaint();
}

BGI_NetLine::~BGI_NetLine() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_NetLine::isSelectable() const noexcept
{
    return mLayer && mLayer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_NetLine::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    // set Z value
    setZValue(getZValueOfCopperLayer(mNetLine.getLayer().getId()));

    mLayer = &mNetLine.getLayer();
    Q_ASSERT(mLayer);

    mLineF.setP1(mNetLine.getStartPoint().getPosition().toPxQPointF());
    mLineF.setP2(mNetLine.getEndPoint().getPosition().toPxQPointF());
    mBoundingRect = QRectF(mLineF.p1(), mLineF.p2()).normalized();
    mBoundingRect.adjust(-mNetLine.getWidth().toPx()/2, -mNetLine.getWidth().toPx()/2,
                         mNetLine.getWidth().toPx()/2, mNetLine.getWidth().toPx()/2);
    mShape = QPainterPath();
    mShape.moveTo(mNetLine.getStartPoint().getPosition().toPxQPointF());
    mShape.lineTo(mNetLine.getEndPoint().getPosition().toPxQPointF());
    QPainterPathStroker ps;
    ps.setCapStyle(Qt::RoundCap);
    Length width = (mNetLine.getWidth() > Length(100000) ? mNetLine.getWidth() : Length(100000));
    ps.setWidth(width.toPx());
    mShape = ps.createStroke(mShape);
    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_NetLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    bool highlight = mNetLine.isSelected() || mNetLine.getNetSignal().isHighlighted();

    // draw line
    if (mLayer->isVisible())
    {
        QPen pen(mLayer->getColor(highlight), mNetLine.getWidth().toPx(), Qt::SolidLine, Qt::RoundCap);
        painter->setPen(pen);
        painter->drawLine(mLineF);
    }

#ifdef QT_DEBUG
    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_NetLine::getBoardLayer(int id) const noexcept
{
    return mNetLine.getBoard().getLayerStack().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
