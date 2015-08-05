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
#include "sgi_symbol.h"
#include "../items/si_symbol.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/gencompinstance.h"
#include <librepcbcommon/schematiclayer.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/gencmp/genericcomponent.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_Symbol::SGI_Symbol(SI_Symbol& symbol) noexcept :
    SGI_Base(), mSymbol(symbol), mLibSymbol(symbol.getLibSymbol())
{
    setZValue(Schematic::ZValue_Symbols);

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
        qreal w = polygon->getWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    foreach (const library::SymbolText* text, mLibSymbol.getTexts())
    {
        // create static text properties
        CachedTextProperties_t props;

        // get the text to display
        props.text = text->getText();
        mSymbol.replaceVariablesWithAttributes(props.text, true);

        // calculate font metrics
        props.fontPixelSize = qCeil(text->getHeight().toPx());
        mFont.setPixelSize(props.fontPixelSize);
        QFontMetricsF metrics(mFont);
        props.scaleFactor = text->getHeight().toPx() / metrics.height();
        props.textRect = metrics.boundingRect(QRectF(), text->getAlign().toQtAlign() |
                                              Qt::TextDontClip, props.text);
        QRectF scaledTextRect = QRectF(props.textRect.topLeft() * props.scaleFactor,
                                       props.textRect.bottomRight() * props.scaleFactor);

        // check rotation
        Angle absAngle = text->getRotation() + mSymbol.getRotation();
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

        // calculate text position
        scaledTextRect.translate(text->getPosition().toPxQPointF());

        // text alignment
        if (props.rotate180)
            props.flags = text->getAlign().mirrored().toQtAlign() | Qt::TextWordWrap;
        else
            props.flags = text->getAlign().toQtAlign() | Qt::TextWordWrap;

        // calculate text bounding rect
        mBoundingRect = mBoundingRect.united(scaledTextRect);
        props.textRect = QRectF(scaledTextRect.topLeft() / props.scaleFactor,
                                scaledTextRect.bottomRight() / props.scaleFactor);
        if (props.rotate180)
        {
            props.textRect = QRectF(-props.textRect.x(), -props.textRect.y(),
                                    -props.textRect.width(), -props.textRect.height()).normalized();
        }

        // save properties
        mCachedTextProperties.insert(text, props);
    }

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_Symbol::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const SchematicLayer* layer = 0;
    const bool selected = mSymbol.isSelected();
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    foreach (const library::SymbolPolygon* polygon, mLibSymbol.getPolygons())
    {
        // set colors
        layer = getSchematicLayer(polygon->getLayerId());
        if (!layer) {if (!layer->isVisible()) layer = nullptr;}
        if (layer)
            painter->setPen(QPen(layer->getColor(selected), polygon->getWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);
        if (polygon->isFilled())
            layer = getSchematicLayer(polygon->getLayerId());
        else if (polygon->isGrabArea())
            layer = getSchematicLayer(SchematicLayer::LayerID::SymbolGrabAreas);
        else
            layer = nullptr;
        if (layer) {if (!layer->isVisible()) layer = nullptr;}
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon->toQPainterPathPx());
    }

    // draw all ellipses
    foreach (const library::SymbolEllipse* ellipse, mLibSymbol.getEllipses())
    {
        // set colors
        layer = getSchematicLayer(ellipse->getLayerId());
        if (layer) {if (!layer->isVisible()) layer = nullptr;}
        if (layer)
            painter->setPen(QPen(layer->getColor(selected), ellipse->getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);
        if (ellipse->isFilled())
            layer = getSchematicLayer(ellipse->getLayerId());
        else if (ellipse->isGrabArea())
            layer = getSchematicLayer(SchematicLayer::LayerID::SymbolGrabAreas);
        else
            layer = nullptr;
        if (layer) {if (!layer->isVisible()) layer = nullptr;}
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
        layer = getSchematicLayer(text->getLayerId());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(text);
        mFont.setPixelSize(props.fontPixelSize);

        // draw text or rect
        painter->save();
        painter->scale(props.scaleFactor, props.scaleFactor);
        if (props.rotate180)
            painter->rotate(-text->getRotation().toDeg() + 180);
        else
            painter->rotate(-text->getRotation().toDeg());
        if ((deviceIsPrinter) || (lod * text->getHeight().toPx() > 8))
        {
            // draw text
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setFont(mFont);
            painter->drawText(props.textRect, props.flags, props.text);
        }
        else
        {
            // fill rect
            painter->fillRect(props.textRect, QBrush(layer->getColor(selected), Qt::Dense5Pattern));
        }
#ifdef QT_DEBUG
        layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsTextsBoundingRect); Q_ASSERT(layer);
        if (layer->isVisible())
        {
            // draw text bounding rect
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(props.textRect);
        }
#endif
        painter->restore();
    }

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = getSchematicLayer(SchematicLayer::OriginCrosses); Q_ASSERT(layer);
        if (layer->isVisible())
        {
            qreal width = Length(700000).toPx();
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->drawLine(-2*width, 0, 2*width, 0);
            painter->drawLine(0, -2*width, 0, 2*width);
        }
    }

#ifdef QT_DEBUG
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GenCompSymbolsCount); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // show symbols count of the generic component
        int count = mSymbol.getGenCompInstance().getPlacedSymbolsCount();
        int maxCount = mSymbol.getGenCompInstance().getSymbolVariant().getItems().count();
        mFont.setPixelSize(Length(1000000).toPx());
        painter->setFont(mFont);
        painter->setPen(QPen(layer->getColor(selected), 0, Qt::SolidLine, Qt::RoundCap));
        painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip,
                          QString("[%1/%2]").arg(count).arg(maxCount));
    }
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsBoundingRect); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(selected), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicLayer* SGI_Symbol::getSchematicLayer(int id) const noexcept
{
    return mSymbol.getSchematic().getProject().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
