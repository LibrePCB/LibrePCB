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
#include "bgi_footprint.h"
#include "../items/bi_footprint.h"
#include "../board.h"
#include "../../project.h"
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"
#include <eda4ulibrary/fpt/footprint.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Footprint::BGI_Footprint(BI_Footprint& footprint) noexcept :
    BGI_Base(), mFootprint(footprint), mLibFootprint(footprint.getLibFootprint())
{
    setZValue(Board::ZValue_FootprintsBottom);

    updateCacheAndRepaint();
}

BGI_Footprint::~BGI_Footprint() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Footprint::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    mBoundingRect = QRectF();
    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);

    // cross rect
    QRectF crossRect(-4, -4, 8, 8);
    mBoundingRect = mBoundingRect.united(crossRect);
    mShape.addRect(crossRect);

    // polygons
    /*foreach (const library::SymbolPolygon* polygon, mLibSymbol.getPolygons())
    {
        QPainterPath polygonPath = polygon->toQPainterPathPx();
        qreal w = polygon->getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea()) mShape = mShape.united(polygonPath);
    }*/

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Footprint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    //const bool selected = mFootprint.isSelected();
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    //const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    /*foreach (const library::SymbolPolygon* polygon, mLibSymbol.getPolygons())
    {
        // set colors
        layer = getSchematicLayer(polygon->getLineLayerId());
        if (layer)
            painter->setPen(QPen(layer->getColor(selected), polygon->getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);
        layer = getSchematicLayer(polygon->getFillLayerId());
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon->toQPainterPathPx());
    }*/

    // draw origin cross
    if (!deviceIsPrinter)
    {
        //layer = getSchematicLayer(SchematicLayer::OriginCrosses);
        //if (layer)
        {
            qreal width = Length(700000).toPx();
            painter->setPen(QPen(/*layer->getColor(selected)*/Qt::red, 0));
            painter->drawLine(-2*width, 0, 2*width, 0);
            painter->drawLine(0, -2*width, 0, 2*width);
        }
    }

/*#ifdef QT_DEBUG
    if (Workspace::instance().getSettings().getDebugTools()->getShowGenCompSymbolCount())
    {
        // show symbols count of the generic component
        layer = getSchematicLayer(SchematicLayer::Busses);
        if (layer)
        {
            uint count = mSymbol.getGenCompInstance().getPlacedSymbolsCount();
            uint maxCount = mSymbol.getGenCompInstance().getSymbolVariant().getItems().count();
            mFont.setPixelSize(Length(1000000).toPx());
            painter->setFont(mFont);
            painter->setPen(QPen(layer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));
            painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip,
                              QString("[%1/%2]").arg(count).arg(maxCount));
        }
    }
    if (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect())
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif*/
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
