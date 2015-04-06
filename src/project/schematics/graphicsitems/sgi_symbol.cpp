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
    mStaticTextProperties.clear();
    foreach (const library::SymbolText* text, mLibSymbol.getTexts())
    {
        // get the text to display
        QString textStr = text->getText();
        mSymbol.replaceVariablesWithAttributes(textStr, true);

        // create static text properties
        StaticTextProperties_t props;
        props.text.setPerformanceHint(QStaticText::AggressiveCaching);
        props.text.setText(textStr.replace("\n", "<br>"));

        // calculate font metrics
        mFont.setPointSizeF(text->getHeight().toPx());
        props.text.prepare(QTransform(), mFont);
        props.fontSize = text->getHeight().toPx()*0.8*text->getHeight().toPx()/props.text.size().height();
        mFont.setPointSizeF(props.fontSize);
        props.text.prepare(QTransform(), mFont);

        // check rotation
        Angle absAngle = text->getAngle() + mSymbol.getAngle();
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle < -Angle::deg90() || absAngle >= Angle::deg90());

        // calculate text position
        if (text->getAlign().getV() == VAlign::top())
            props.origin.setY(text->getPosition().toPxQPointF().y());
        else if (text->getAlign().getV() == VAlign::bottom())
            props.origin.setY(text->getPosition().toPxQPointF().y()-props.text.size().height());
        else
            props.origin.setY(text->getPosition().toPxQPointF().y()-props.text.size().height()/2);
        if (text->getAlign().getH() == HAlign::left())
            props.origin.setX(text->getPosition().toPxQPointF().x());
        else if (text->getAlign().getH() == HAlign::right())
            props.origin.setX(text->getPosition().toPxQPointF().x()-props.text.size().width());
        else
            props.origin.setX(text->getPosition().toPxQPointF().x()-props.text.size().width()/2);
        props.textRect = QRectF(props.origin.x(), props.origin.y(), props.text.size().width(), props.text.size().height());
        if (props.rotate180)
        {
            props.origin.setX(-props.origin.x()-props.text.size().width());
            props.origin.setY(-props.origin.y()-props.text.size().height());
        }

        mStaticTextProperties.insert(text, props);
        mBoundingRect = mBoundingRect.united(props.textRect);
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

        // get static text properties
        const StaticTextProperties_t& props = mStaticTextProperties.value(text);
        mFont.setPointSizeF(props.fontSize);

        // draw text or rect
        if ((deviceIsPrinter) || (lod * text->getHeight().toPx() > 10))
        {
            // draw text
            painter->save();
            if (props.rotate180)
                painter->rotate(text->getAngle().toDeg() + 180);
            else
                painter->rotate(text->getAngle().toDeg());
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setFont(mFont);
            painter->drawStaticText(props.origin, props.text);
            painter->restore();
        }
        else
        {
            // fill rect
            painter->fillRect(props.textRect, QBrush(layer->getColor(selected), Qt::Dense5Pattern));
        }
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
