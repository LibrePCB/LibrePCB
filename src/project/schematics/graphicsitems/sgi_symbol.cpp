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
#include "sgi_symbol.h"
#include "../items/si_symbol.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/gencompinstance.h"
#include "../../../common/schematiclayer.h"
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"
#include "../../../library/symbol.h"
#include "../../../library/genericcomponent.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_Symbol::SGI_Symbol(SI_Symbol& symbol) noexcept :
    SGI_Base(), mSymbol(symbol), mLibSymbol(symbol.getLibSymbol())
{
    setZValue(Schematic::ZValue_Symbols);
    setFlags(QGraphicsItem::ItemIsSelectable);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

    updateCacheAndRepaint();
}

SGI_Symbol::~SGI_Symbol() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SGI_Symbol::updateCacheAndRepaint() noexcept
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
    foreach (const library::SymbolPolygon* polygon, mLibSymbol.getPolygons())
    {
        QPainterPath polygonPath = polygon->toQPainterPathPx();
        qreal w = polygon->getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    foreach (const library::SymbolText* text, mLibSymbol.getTexts())
    {
        // calculate font metrics
        mFont.setPixelSize(50);
        QFontMetricsF metrics(mFont);
        qreal factor = 0.8*text->getHeight().toPx() / metrics.height();

        // check rotation
        Angle absAngle = text->getAngle() + mSymbol.getAngle();
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
        mSymbol.replaceVariablesWithAttributes(textStr, true);

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
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_Symbol::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    QPen pen;
    const SchematicLayer* layer = 0;
    const bool selected = option->state & QStyle::State_Selected;
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    foreach (const library::SymbolPolygon* polygon, mLibSymbol.getPolygons())
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
        painter->drawPath(polygon->toQPainterPathPx());
    }

    // draw all ellipses
    foreach (const library::SymbolEllipse* ellipse, mLibSymbol.getEllipses())
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
    foreach (const library::SymbolText* text, mLibSymbol.getTexts())
    {
        // get layer
        layer = getSchematicLayer(text->getLayerId()); if (!layer) continue;

        // calculate font metrics
        mFont.setPixelSize(50);
        QFontMetricsF metrics(mFont);
        qreal factor = 0.8*text->getHeight().toPx() / metrics.height();

        // check rotation
        Angle absAngle = text->getAngle() + mSymbol.getAngle();
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
        mSymbol.replaceVariablesWithAttributes(textStr, true);

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
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGenCompSymbolCount()))
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
 *  Private Methods
 ****************************************************************************************/

SchematicLayer* SGI_Symbol::getSchematicLayer(uint id) const noexcept
{
    return mSymbol.getSchematic().getProject().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
