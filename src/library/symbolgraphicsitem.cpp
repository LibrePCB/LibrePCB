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
#include "../project/circuit/gencompinstance.h"
#include "../workspace/settings/workspacesettings.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolGraphicsItem::SymbolGraphicsItem(const Symbol& symbol,
                                       project::SymbolInstance* instance) throw (Exception) :
    QGraphicsItem(0), mSymbol(symbol), mSymbolInstance(instance)
{
    setZValue(project::Schematic::ZValue_Symbols);
    if (mSymbolInstance) setFlags(QGraphicsItem::ItemIsSelectable);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

    try
    {
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
        throw;
    }

    updateCacheAndRepaint();
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

    QPen pen;
    const SchematicLayer* layer = 0;
    const bool selected = option->state & QStyle::State_Selected;
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    foreach (const SymbolPolygon* polygon, mSymbol.getPolygons())
    {
        // set colors
        layer = getSchematicLayer(polygon->getLineLayerId());
        if (layer)
        {
            pen = QPen(layer->getColor(selected), polygon->getLineWidth().toPx() * lod, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            pen.setCosmetic(true);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        layer = getSchematicLayer(polygon->getFillLayerId());
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        QPainterPath polygonPath;
        polygonPath.setFillRule(Qt::WindingFill);
        Point lastPos = polygon->getStartPos();
        polygonPath.moveTo(lastPos.toPxQPointF());
        foreach (const SymbolPolygonSegment* segment, polygon->getSegments())
        {
            if (segment->getAngle() == 0)
            {
                polygonPath.lineTo(segment->getEndPos().toPxQPointF());
            }
            else
            {
                // TODO: this is very provisional and may contain bugs...
                // all lengths in pixels
                qreal s = Point(segment->getEndPos() - lastPos).getLength().toPx();
                qreal r = s / (2 * qSin(segment->getAngle().toRad()/2));
                qreal x1 = lastPos.toPxQPointF().x();
                qreal y1 = lastPos.toPxQPointF().y();
                qreal x2 = segment->getEndPos().toPxQPointF().x();
                qreal y2 = segment->getEndPos().toPxQPointF().y();
                qreal x3 = (x1+x2)/2;
                qreal y3 = (y1+y2)/2;
                qreal q = qSqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
                qreal cx = x3+ qSqrt(r*r-q*q/4)*(y1-y2)/q;
                qreal cy = y3 + qSqrt(r*r-q*q/4)*(x2-x1)/q;
                QRectF rect(cx-r, cy-r, 2*r, 2*r);
                qreal startAngleDeg = qRadiansToDegrees(qAtan2(cy-y1, cx-x1));
                polygonPath.arcTo(rect, startAngleDeg, -segment->getAngle().toDeg());
            }
            lastPos = segment->getEndPos();
        }
        painter->drawPath(polygonPath);
    }

    // draw all ellipses
    foreach (const SymbolEllipse* ellipse, mSymbol.getEllipses())
    {
        // set colors
        layer = getSchematicLayer(ellipse->getLineLayerId()); if (!layer) continue;
        if (layer)
        {
            pen = QPen(layer->getColor(selected), ellipse->getLineWidth().toPx() * lod, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            pen.setCosmetic(true);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        layer = getSchematicLayer(ellipse->getFillLayerId());
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw ellipse
        painter->drawEllipse(ellipse->getCenter().toPxQPointF(), ellipse->getRadiusX().toPx(),
                             ellipse->getRadiusY().toPx());
        // TODO: rotation
    }

    // draw all texts
    foreach (const SymbolText* text, mSymbol.getTexts())
    {
        // get layer
        layer = getSchematicLayer(text->getLayerId()); if (!layer) continue;

        // calculate font metrics
        mFont.setPixelSize(50);
        QFontMetricsF metrics(mFont);
        qreal factor = 0.8*text->getHeight().toPx() / metrics.height();

        // check rotation
        Angle absAngle = text->getAngle();
        if (mSymbolInstance) absAngle += mSymbolInstance->getAngle();
        absAngle.mapTo180deg();
        bool rotate180 = (absAngle < -Angle::deg90() || absAngle >= Angle::deg90());

        // calculate text rect
        qreal x, y;
        if (text->getAlign().getV() == VAlign::top())
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor+0.1*text->getHeight().toPx()/factor;
        }
        else if (text->getAlign().getV() == VAlign::bottom())
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor-0.1*text->getHeight().toPx()/factor;
        }
        else
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor;
        }
        if (rotate180)
        {
            x = -x;
            y = -y;
        }

        // get flags
        int flags = text->getAlign().toQtAlign() | Qt::TextDontClip;
        if (rotate180)
        {
            if (flags & Qt::AlignLeft) {flags &= ~Qt::AlignLeft; flags |= Qt::AlignRight;}
            else if (flags & Qt::AlignRight) {flags &= ~Qt::AlignRight; flags |= Qt::AlignLeft;}
            if (flags & Qt::AlignBottom) {flags &= ~Qt::AlignBottom; flags |= Qt::AlignTop;}
            else if (flags & Qt::AlignTop) {flags &= ~Qt::AlignTop; flags |= Qt::AlignBottom;}
        }

        // get the text to display
        QString textStr = text->getText();
        if (mSymbolInstance)
            mSymbolInstance->replaceVariablesWithAttributes(textStr, true);

        // draw text or rect
        painter->save();
        painter->scale(factor, factor);
        if (rotate180)
            painter->rotate(text->getAngle().toDeg() + (qreal)180);
        else
            painter->rotate(text->getAngle().toDeg());
        if ((deviceIsPrinter) || (lod * text->getHeight().toPx() > 10))
        {
            // draw text
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setFont(mFont);
            painter->drawText(QRectF(x, y, 0, 0), flags, textStr);
        }
        else
        {
            // fill rect
            QRectF textRect = metrics.boundingRect(QRectF(x, y, 0, 0), flags, textStr);
            painter->fillRect(textRect, QBrush(layer->getColor(selected), Qt::Dense5Pattern));
        }
        painter->restore();
    }

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = getSchematicLayer(SchematicLayer::OriginCrosses);
        if (layer)
        {
            qreal width = Length(700000).toPx();
            pen = QPen(layer->getColor(selected), 0);
            painter->setPen(pen);
            painter->drawLine(-2*width, 0, 2*width, 0);
            painter->drawLine(0, -2*width, 0, 2*width);
        }
    }

#ifdef QT_DEBUG
    if ((mSymbolInstance) && (!deviceIsPrinter) &&
        (Workspace::instance().getSettings().getDebugTools()->getShowGenCompSymbolCount()))
    {
        // show symbols count of the generic component
        layer = getSchematicLayer(SchematicLayer::Busses);
        if (layer)
        {
            unsigned int count = mSymbolInstance->getGenCompInstance().getPlacedSymbolsCount();
            unsigned int maxCount = mSymbolInstance->getGenCompInstance().getSymbolVariant().getItems().count();
            mFont.setPixelSize(Length(1000000).toPx());
            painter->setFont(mFont);
            painter->setPen(QPen(layer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));
            painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip,
                              QString("[%1/%2]").arg(count).arg(maxCount));
        }
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
 *  General Methods
 ****************************************************************************************/

void SymbolGraphicsItem::updateCacheAndRepaint() noexcept
{
    mBoundingRect = QRectF();
    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);

    // cross rect
    QRectF crossRect(-4, -4, 8, 8);
    mBoundingRect = mBoundingRect.united(crossRect);
    mShape.addRect(crossRect);

    // polygons
    foreach (const SymbolPolygon* polygon, mSymbol.getPolygons())
    {
        Point lastPos = polygon->getStartPos();
        QPainterPath polygonPath;
        polygonPath.setFillRule(Qt::WindingFill);
        polygonPath.moveTo(lastPos.toPxQPointF());
        foreach (const SymbolPolygonSegment* segment, polygon->getSegments())
        {
            if (segment->getAngle() == 0)
            {
                polygonPath.lineTo(segment->getEndPos().toPxQPointF());
            }
            else
            {
                //TODO
            }
            lastPos = segment->getEndPos();
        }
        qreal w = polygon->getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea())
            mShape = mShape.united(polygonPath);
    }

    // texts
    foreach (const SymbolText* text, mSymbol.getTexts())
    {
        // calculate font metrics
        mFont.setPixelSize(50);
        QFontMetricsF metrics(mFont);
        qreal factor = 0.8*text->getHeight().toPx() / metrics.height();

        // check rotation
        Angle absAngle = text->getAngle();
        if (mSymbolInstance) absAngle += mSymbolInstance->getAngle();
        absAngle.mapTo180deg();
        bool rotate180 = (absAngle < -Angle::deg90() || absAngle >= Angle::deg90());

        // calculate text rect
        qreal x, y;
        if (text->getAlign().getV() == VAlign::top())
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor+0.1*text->getHeight().toPx()/factor;
        }
        else if (text->getAlign().getV() == VAlign::bottom())
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor-0.1*text->getHeight().toPx()/factor;
        }
        else
        {
            x = text->getPosition().toPxQPointF().x()/factor;
            y = text->getPosition().toPxQPointF().y()/factor;
        }
        if (rotate180)
        {
            x = -x;
            y = -y;
        }

        // get flags
        int flags = text->getAlign().toQtAlign() | Qt::TextDontClip;
        if (rotate180)
        {
            if (flags & Qt::AlignLeft) {flags &= ~Qt::AlignLeft; flags |= Qt::AlignRight;}
            else if (flags & Qt::AlignRight) {flags &= ~Qt::AlignRight; flags |= Qt::AlignLeft;}
            if (flags & Qt::AlignBottom) {flags &= ~Qt::AlignBottom; flags |= Qt::AlignTop;}
            else if (flags & Qt::AlignTop) {flags &= ~Qt::AlignTop; flags |= Qt::AlignBottom;}
        }

        // get the text to display
        QString textStr = text->getText();
        if (mSymbolInstance)
            mSymbolInstance->replaceVariablesWithAttributes(textStr, true);

        QRectF textRect = metrics.boundingRect(QRectF(x, y, 0, 0), flags, textStr).normalized();
        if (rotate180)
            textRect = QRectF(-textRect.left() * factor, -textRect.top() * factor, -textRect.width() * factor, -textRect.height() * factor).normalized();
        else
            textRect = QRectF(textRect.left() * factor, textRect.top() * factor, textRect.width() * factor, textRect.height() * factor).normalized();
        mBoundingRect = mBoundingRect.united(textRect);
    }

    update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

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
