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
#include <librepcblibrary/pkg/footprintpad.h>
#include "../../settings/projectsettings.h"
#include "../deviceinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad) noexcept :
    BGI_Base(), mPad(pad), mLibPad(pad.getLibPad())
{
    //QStringList localeOrder = mPad.getProject().getSettings().getLocaleOrder();
    //setToolTip(mLibPad.getName(localeOrder) % ": " % mLibPad.getDescription(localeOrder));

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");
    mFont.setPixelSize(5);

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
    mShape.setFillRule(Qt::WindingFill);
    mBoundingRect = QRectF();

    // set Z value
    if ((mLibPad.getType() == library::FootprintPad::Type_t::SmtRect) && (mPad.getIsMirrored()))
        setZValue(Board::ZValue_FootprintPadsBottom);
    else
        setZValue(Board::ZValue_FootprintPadsTop);

    // rotation
    Angle absAngle = mLibPad.getRotation() + mPad.getFootprint().getRotation();
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    QRectF rect = QRectF(-mLibPad.getWidth().toPx()/2, -mLibPad.getHeight().toPx()/2,
                         mLibPad.getWidth().toPx(), mLibPad.getHeight().toPx());
    mShape.addRect(rect);
    mBoundingRect = mBoundingRect.united(mShape.boundingRect());

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

    BoardLayer* layer;
    if (mLibPad.getType() == library::FootprintPad::Type_t::SmtRect)
        layer = getBoardLayer(BoardLayer::LayerID::TopCopper);
    else
        layer = getBoardLayer(BoardLayer::LayerID::Vias);
    Q_ASSERT(layer);

    if (layer->isVisible())
    {
        painter->setPen(QPen(layer->getColor(mPad.isSelected()), 0));
        painter->setBrush(layer->getColor(mPad.isSelected()));

        QRectF rect = QRectF(-mLibPad.getWidth().toPx()/2, -mLibPad.getHeight().toPx()/2,
                             mLibPad.getWidth().toPx(), mLibPad.getHeight().toPx());
        switch (mLibPad.getType())
        {
            case library::FootprintPad::Type_t::ThtRect:
            case library::FootprintPad::Type_t::SmtRect:
                painter->drawRect(rect);
                break;
            case library::FootprintPad::Type_t::ThtOctagon:
            {
                qreal rx = mLibPad.getWidth().toPx()/2;
                qreal ry = mLibPad.getHeight().toPx()/2;
                qreal a = qMin(rx, ry) * (2 - qSqrt(2));
                QPolygonF octagon;
                octagon.append(QPointF(rx, ry-a));
                octagon.append(QPointF(rx-a, ry));
                octagon.append(QPointF(a-rx, ry));
                octagon.append(QPointF(-rx, ry-a));
                octagon.append(QPointF(-rx, a-ry));
                octagon.append(QPointF(a-rx, -ry));
                octagon.append(QPointF(rx-a, -ry));
                octagon.append(QPointF(rx, a-ry));
                painter->drawPolygon(octagon);
                break;
            }
            case library::FootprintPad::Type_t::ThtRound:
            {
                qreal radius = qMin(mLibPad.getWidth().toPx(), mLibPad.getHeight().toPx())/2;
                painter->drawRoundedRect(rect, radius, radius);
                break;
            }
            default: Q_ASSERT(false); break;
        }
    }

#ifdef QT_DEBUG
    layer = getBoardLayer(BoardLayer::LayerID::DEBUG_GraphicsItemsBoundingRect); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(mPad.isSelected()), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_FootprintPad::getBoardLayer(int id) const noexcept
{
    if (mPad.getIsMirrored()) id = BoardLayer::getMirroredLayerId(id);
    return mPad.getFootprint().getDeviceInstance().getBoard().getProject().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
