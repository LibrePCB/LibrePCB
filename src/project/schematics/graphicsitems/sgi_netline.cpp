/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "../../../common/schematiclayer.h"
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_NetLine::SGI_NetLine(SI_NetLine& netline) noexcept :
    SGI_Base(), mNetLine(netline), mLayer(nullptr)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetLines);

    mLayer = mNetLine.getSchematic().getProject().getSchematicLayer(SchematicLayer::Nets);
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
    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_NetLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const bool selected = option->state & QStyle::State_Selected;
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw line
    QPen pen(mLayer->getColor(selected), mNetLine.getWidth().toPx() * lod, Qt::SolidLine, Qt::RoundCap);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(mLineF);

#ifdef QT_DEBUG
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowSchematicNetlinesNetsignals()))
    {
        // draw net signal name
        QFont font;
        font.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("Monospace");
        font.setPixelSize(3);
        painter->setFont(font);
        painter->setPen(QPen(mLayer->getColor(selected), 0));
        painter->drawText(mLineF.pointAt((qreal)0.5), mNetLine.getNetSignal()->getName());
    }
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect()))
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
