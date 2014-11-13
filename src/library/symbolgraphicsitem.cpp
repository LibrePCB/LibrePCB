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
#include <QPrinter>
#include "symbolgraphicsitem.h"
#include "symbol.h"
#include "genericcomponent.h"
#include "../project/schematics/symbolinstance.h"
#include "../common/schematiclayer.h"
#include "../project/schematics/schematic.h"
#include "../project/project.h"
#include "../project/schematics/schematic.h"
#include "../project/schematics/symbolpininstance.h"
#include "../project/circuit/gencompsignalinstance.h"
#include "../project/circuit/netsignal.h"
#include "symbolpingraphicsitem.h"
#include "../workspace/workspace.h"
#include "../project/circuit/genericcomponentinstance.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolGraphicsItem::SymbolGraphicsItem(const Symbol& symbol,
                                       project::SymbolInstance* instance) throw (Exception) :
    QGraphicsItem(0), mSymbol(symbol), mSymbolInstance(instance)
{
    setZValue(project::Schematic::ZValue_Symbols);
    if (mSymbolInstance)
        setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsScenePositionChanges);

    try
    {
        if (!updateBoundingRectAndShape())
        {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                QApplication::translate("SymbolGraphicsItem", "Invalid Symbol"));
        }

        // add pins
        foreach (const SymbolPin* pin, mSymbol.getPins())
        {
            if (mSymbolInstance)
            {
                project::SymbolPinInstance* instance = mSymbolInstance->getPinInstance(pin->getUuid());
                if (!instance) throw LogicError(__FILE__, __LINE__, pin->getUuid().toString());
                mPinItems.insert(pin->getUuid(), new SymbolPinGraphicsItem(*this, *pin, instance));
            }
            else
                mPinItems.insert(pin->getUuid(), new SymbolPinGraphicsItem(*this, *pin, 0));
        }
    }
    catch (Exception& e)
    {
        qDeleteAll(mPinItems);      mPinItems.clear();
    }
}

SymbolGraphicsItem::~SymbolGraphicsItem() noexcept
{
    qDeleteAll(mPinItems);      mPinItems.clear();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SymbolGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                               QWidget* widget)
{
    Q_UNUSED(widget);

    SchematicLayer* layer = 0;
    bool selected = option->state & QStyle::State_Selected;
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = getSchematicLayer(SchematicLayer::OriginCrosses);
        if (layer)
        {
            QPen pen(layer->getColor(selected), 2);
            pen.setCosmetic(true);
            painter->setPen(pen);
            painter->drawLine(-4, 0, 4, 0);
            painter->drawLine(0, -4, 0, 4);
        }
    }

    // draw all polygons
    foreach (const SymbolPolygon* polygon, mSymbol.getPolygons())
    {
        // set colors
        layer = getSchematicLayer(polygon->getLayerId());
        if (!layer) continue;
        painter->setPen(QPen(layer->getColor(selected), polygon->getWidth().toPx(), Qt::SolidLine, Qt::RoundCap));
        if (polygon->getFill())
            painter->setBrush(QBrush(layer->getFillColor(selected), Qt::SolidPattern));
        else
            painter->setBrush(Qt::NoBrush);

        // draw polygon
        QPainterPath polygonPath;
        polygonPath.setFillRule(Qt::WindingFill);
        polygonPath.moveTo(polygon->getStartPos().toPxQPointF());
        foreach (const SymbolPolygon::PolygonSegment_t* segment, polygon->getSegments())
        {
            switch (segment->type)
            {
                case SymbolPolygon::PolygonSegment_t::Line:
                    polygonPath.lineTo(segment->endPos.toPxQPointF());
                    break;
                case SymbolPolygon::PolygonSegment_t::Arc:
                    //polygonPath.arcTo(item->getArcRectF(), item->getArcStartAngle().deg(),
                    //                                      item->getArcSpanAngle().deg());
                    break;
                default:
                    break;
            }
        }
        painter->drawPath(polygonPath);
    }

    // draw all texts
    foreach (const SymbolText* text, mSymbol.getTexts())
    {
        // set colors
        layer = getSchematicLayer(text->getLayerId());
        if (!layer) continue;
        painter->setPen(QPen(layer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));

        // calculate font metrics
        QFont font;
        font.setStyleHint(QFont::SansSerif);
        font.setStyleStrategy(QFont::StyleStrategy(QFont::ForceOutline | QFont::PreferMatch));
        font.setPixelSize(text->getHeight().toPx());
        QFontMetricsF metrics(font);
        qreal factor = 0.8*text->getHeight().toPx() / metrics.height();

        // calculate text rect
        QRectF textRect(text->getPosition().toPxQPointF().x()/factor, text->getPosition().toPxQPointF().y()/factor, 0, 0);
        if (text->getAlign() & Qt::AlignTop)
            textRect = QRectF(text->getPosition().toPxQPointF().x()/factor, text->getPosition().toPxQPointF().y()/factor+0.1*text->getHeight().toPx()/factor, 0, 0);
        else if (text->getAlign() & Qt::AlignBottom)
            textRect = QRectF(text->getPosition().toPxQPointF().x()/factor, text->getPosition().toPxQPointF().y()/factor-0.1*text->getHeight().toPx()/factor, 0, 0);

        QString textStr = text->getText();
        if (mSymbolInstance)
        {
            if (textStr == "${NAME}") // TODO: this is only a test...
                textStr = mSymbolInstance->getGenCompInstance().getName();
        }

        // draw text
        painter->save();
        painter->scale(factor, factor);
        painter->setFont(font);
        painter->drawText(textRect, text->getAlign() | Qt::TextSingleLine | Qt::TextDontClip,
                          textStr, &textRect);
        painter->restore();
    }

    // only for debugging: show symbols count of the generic component
    if (mSymbolInstance)
    {
        layer = getSchematicLayer(SchematicLayer::Busses);
        if (layer)
        {
            unsigned int count = mSymbolInstance->getGenCompInstance().getUsedSymbolsCount();
            unsigned int maxCount = mSymbolInstance->getGenCompInstance().getSymbolVariant().getItems().count();
            QFont font;
            font.setFamily("Monospace");
            font.setPixelSize(3);
            font.setStyleHint(QFont::TypeWriter);
            font.setStyleStrategy(QFont::ForceOutline);
            painter->setFont(font);
            painter->setPen(QPen(layer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));
            painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip,
                              QString("[%1/%2]").arg(count).arg(maxCount));
        }
    }
}

QVariant SymbolGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    /*if (scene())
    {
        switch (change)
        {
            case ItemPositionHasChanged:
                break;
            default:
                break;
        }
    }*/

    return QGraphicsItem::itemChange(change, value);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolGraphicsItem::updateBoundingRectAndShape() noexcept
{
    QRectF boundingRect;
    QPainterPath shape;
    shape.setFillRule(Qt::WindingFill);

    // cross rect
    QRectF crossRect(-4, -4, 8, 8);
    boundingRect = boundingRect.united(crossRect);
    shape.addRect(crossRect);

    // polygons
    foreach (const SymbolPolygon* polygon, mSymbol.getPolygons())
    {
        QPainterPath polygonPath;
        polygonPath.setFillRule(Qt::WindingFill);
        polygonPath.moveTo(polygon->getStartPos().toPxQPointF());
        foreach (const SymbolPolygon::PolygonSegment_t* segment, polygon->getSegments())
        {
            switch (segment->type)
            {
                case SymbolPolygon::PolygonSegment_t::Line:
                    polygonPath.lineTo(segment->endPos.toPxQPointF());
                    break;
                case SymbolPolygon::PolygonSegment_t::Arc:
                    //polygonPath.arcTo(item->getArcRectF(), item->getArcStartAngle().deg(),
                    //                                      item->getArcSpanAngle().deg());
                    break;
                default:
                    qWarning() << "Unknown polygon segment type:" << segment->type;
                    return false;
            }
        }
        qreal w = polygon->getWidth().toPx() / 2;
        boundingRect = boundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->getFill())
            shape = shape.united(polygonPath);
    }

    // texts
    /*foreach (const SymbolText* text, mSymbol.getTexts())
    {

    }*/

    mBoundingRect = boundingRect;
    mShape = shape;
    return true;
}

SchematicLayer* SymbolGraphicsItem::getSchematicLayer(unsigned int id) const noexcept
{
    if (mSymbolInstance)
        return mSymbolInstance->getSchematic().getProject().getSchematicLayer(id);
    else
        return Workspace::instance().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
