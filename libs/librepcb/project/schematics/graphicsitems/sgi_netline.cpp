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
#include "sgi_netline.h"
#include "../items/si_netline.h"
#include "../items/si_netpoint.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include <librepcbcommon/schematiclayer.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_NetLine::SGI_NetLine(SI_NetLine& netline) noexcept :
    SGI_Base(), mNetLine(netline), mLayer(nullptr)
{
    setZValue(Schematic::ZValue_NetLines);

    mLayer = getSchematicLayer(SchematicLayer::Nets);
    Q_ASSERT(mLayer);

    updateCacheAndRepaint();
}

SGI_NetLine::~SGI_NetLine() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SGI_NetLine::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();
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
    Length width = (mNetLine.getWidth() > Length(1270000) ? mNetLine.getWidth() : Length(1270000));
    ps.setWidth(width.toPx());
    mShape = ps.createStroke(mShape);
    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_NetLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
    SchematicLayer* layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_NetLinesNetSignalNames); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw net signal name
        QFont font;
        font.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("Monospace");
        font.setPixelSize(3);
        painter->setFont(font);
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->drawText(mLineF.pointAt((qreal)0.5), mNetLine.getNetSignal().getName());
    }
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsBoundingRect); Q_ASSERT(layer);
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

SchematicLayer* SGI_NetLine::getSchematicLayer(int id) const noexcept
{
    return mNetLine.getSchematic().getProject().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
