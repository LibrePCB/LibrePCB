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
#include <QtWidgets>
#include <QPrinter>
#include "bgi_via.h"
#include "../items/bi_via.h"
#include "../board.h"
#include "../../project.h"
#include "../boardlayerstack.h"
#include "../../circuit/netsignal.h"
#include <librepcb/common/boardlayer.h>
#include <librepcb/common/boarddesignrules.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Via::BGI_Via(BI_Via& via) noexcept :
    BGI_Base(), mVia(via), mViaLayer(nullptr), mTopStopMaskLayer(nullptr),
    mBottomStopMaskLayer(nullptr)
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
    return mViaLayer && mViaLayer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Via::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    setToolTip(mVia.getNetSignal() ? mVia.getNetSignal()->getName() : QString());

    mViaLayer = getBoardLayer(BoardLayer::Vias);
    mTopStopMaskLayer = getBoardLayer(BoardLayer::TopStopMask);
    mBottomStopMaskLayer = getBoardLayer(BoardLayer::BottomStopMask);

    // determine stop mask clearance
    mDrawStopMask = mVia.getBoard().getDesignRules().doesViaRequireStopMask(mVia.getDrillDiameter());
    mStopMaskClearance = mVia.getBoard().getDesignRules().calcStopMaskClearance(mVia.getSize());

    // set shape and bounding rect
    qreal shapeRadius = (mVia.getSize()/2).toPx();
    qreal stopMaskRadius = ((mVia.getSize() + mStopMaskClearance*2) / 2).toPx();
    mBoundingRect = QRectF(-stopMaskRadius, -stopMaskRadius, 2*stopMaskRadius, 2*stopMaskRadius);
    mShape = QPainterPath();
    mShape.addEllipse(QRectF(-shapeRadius, -shapeRadius, 2*shapeRadius, 2*shapeRadius));

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Via::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    NetSignal* netsignal = mVia.getNetSignal();
    bool highlight = mVia.isSelected() || (netsignal && netsignal->isHighlighted());

    if (mDrawStopMask && mBottomStopMaskLayer && mBottomStopMaskLayer->isVisible()) {
        // draw bottom stop mask
        painter->setPen(Qt::NoPen);
        painter->setBrush(mBottomStopMaskLayer->getColor(highlight));
        painter->drawPath(mVia.toQPainterPathPx(mStopMaskClearance, false));
    }

    if (mViaLayer && mViaLayer->isVisible()) {
        // draw via
        painter->setPen(Qt::NoPen);
        painter->setBrush(mViaLayer->getColor(highlight));
        painter->drawPath(mVia.toQPainterPathPx(Length(0), true));

        // draw netsignal name
        if (netsignal) {
            painter->setFont(mFont);
            painter->setPen(mViaLayer->getColor(highlight).lighter(150));
            painter->drawText(mBoundingRect, Qt::AlignCenter, netsignal->getName());
        }
    }

    if (mDrawStopMask && mTopStopMaskLayer && mTopStopMaskLayer->isVisible()) {
        // draw top stop mask
        painter->setPen(Qt::NoPen);
        painter->setBrush(mTopStopMaskLayer->getColor(highlight));
        painter->drawPath(mVia.toQPainterPathPx(mStopMaskClearance, false));
    }

#ifdef QT_DEBUG
    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible()) {
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
