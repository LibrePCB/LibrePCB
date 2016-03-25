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
#include "bgi_via.h"
#include "../items/bi_via.h"
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

BGI_Via::BGI_Via(BI_Via& via) noexcept :
    BGI_Base(), mVia(via), mLayer(nullptr)
{
    setZValue(Board::ZValue_Vias);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Helvetica");
    mFont.setPixelSize(1);

    updateCacheAndRepaint();
}

BGI_Via::~BGI_Via() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_Via::isSelectable() const noexcept
{
    return mLayer && mLayer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Via::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    setToolTip(mVia.getNetSignal() ? mVia.getNetSignal()->getName() : QString());

    mLayer = getBoardLayer(BoardLayer::Vias);
    Q_ASSERT(mLayer);
    if (!mLayer->isVisible()) {
        mLayer = nullptr;
    }

    // set shape and bounding rect
    if (mLayer) {
        qreal radius = mVia.getSize().toPx() / 2;
        mBoundingRect = QRectF(-radius, -radius, 2*radius, 2*radius);
        mShape = QPainterPath();
        mShape.addEllipse(mBoundingRect);
    } else {
        mBoundingRect = QRectF(0, 0, 0, 0);
        mShape = QPainterPath();
    }

    setVisible(!mBoundingRect.isEmpty());

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Via::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!mLayer) return;

    NetSignal* netsignal = mVia.getNetSignal();
    bool highlight = mVia.isSelected() || (netsignal && netsignal->isHighlighted());

    // draw via
    painter->setPen(Qt::NoPen);
    painter->setBrush(mLayer->getColor(highlight));
    painter->drawPath(mVia.toQPainterPathPx());

    // draw netsignal name
    if (netsignal) {
        painter->setFont(mFont);
        painter->setPen(mLayer->getColor(highlight).lighter(150));
        painter->drawText(mBoundingRect, Qt::AlignCenter, netsignal->getName());
    }

#ifdef QT_DEBUG
    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_Via::getBoardLayer(int id) const noexcept
{
    return mVia.getBoard().getLayerStack().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
