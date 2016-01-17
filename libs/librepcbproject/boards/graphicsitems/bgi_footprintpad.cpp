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
#include "bgi_footprintpad.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_footprint.h"
#include "../board.h"
#include "../../project.h"
#include <librepcbcommon/boardlayer.h>
#include <librepcblibrary/pkg/footprint.h>
#include "../../settings/projectsettings.h"
#include "../deviceinstance.h"
#include <librepcblibrary/pkg/package.h>
#include "../boardlayerstack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad) noexcept :
    BGI_Base(), mPad(pad), mLibPad(pad.getLibPad()), mLibPkgPad(nullptr)
{
    mLibPkgPad = mPad.getFootprint().getDeviceInstance().getLibPackage().getPadByUuid(pad.getLibPadUuid());
    Q_ASSERT(mLibPkgPad);
    setToolTip(mLibPkgPad->getName());

    updateCacheAndRepaint();
}

BGI_FootprintPad::~BGI_FootprintPad() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_FootprintPad::updateCacheAndRepaint() noexcept
{
    mShape = QPainterPath();
    mBoundingRect = QRectF();

    // set Z value
    if (mLibPad.getTechnology() == library::FootprintPad::Technology_t::SMT) {
        const library::FootprintPadSmt* smt = dynamic_cast<const library::FootprintPadSmt*>(&mLibPad);
        Q_ASSERT(smt);
        if ((smt->getBoardSide() == library::FootprintPadSmt::BoardSide_t::BOTTOM) != mPad.getIsMirrored())
            setZValue(Board::ZValue_FootprintPadsBottom);
        else
            setZValue(Board::ZValue_FootprintPadsTop);
    } else {
        setZValue(Board::ZValue_FootprintPadsTop);
    }

    // set layer
    mPadLayer = getBoardLayer(mLibPad.getLayerId());
    if (mPadLayer) {
        if (!mPadLayer->isVisible())
            mPadLayer = nullptr;
    }

    // set shape and bounding rect
    if (mPadLayer) {
        mShape.addRect(mLibPad.getBoundingRectPx());
        mBoundingRect = mBoundingRect.united(mShape.boundingRect());
    }

    if (!mShape.isEmpty())
        mShape.setFillRule(Qt::WindingFill);

    setVisible(!mBoundingRect.isEmpty());

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_FootprintPad::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    //const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    //const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (!mPadLayer) return;

    // draw pad
    painter->setPen(Qt::NoPen);
    painter->setBrush(mPadLayer->getColor(mPad.isSelected()));
    painter->drawPath(mLibPad.toQPainterPathPx());

#ifdef QT_DEBUG
    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRects);
    if (layer) {
        if (layer->isVisible()) {
            // draw bounding rect
            painter->setPen(QPen(layer->getColor(mPad.isSelected()), 0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(mBoundingRect);
        }
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_FootprintPad::getBoardLayer(int id) const noexcept
{
    if (mPad.getIsMirrored()) id = BoardLayer::getMirroredLayerId(id);
    return mPad.getFootprint().getDeviceInstance().getBoard().getLayerStack().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
