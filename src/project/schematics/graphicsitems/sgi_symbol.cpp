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
#include "../../../library/sym/symbol.h"
#include "../../../library/gencmp/genericcomponent.h"

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
        qreal w = polygon->getLineWidth().toPx() / 2;
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
        mFont.setPointSizeF(text->getHeight().toPx());
        QFontMetricsF metrics(mFont);
        props.fontSize = text->getHeight().toPx()*0.8*text->getHeight().toPx()/metrics.height();
        mFont.setPointSizeF(props.fontSize);
        metrics = QFontMetricsF(mFont);
        props.textRect = metrics.boundingRect(QRectF(), text->getAlign().toQtAlign() |
                                              Qt::TextDontClip, props.text);

        // check rotation
        Angle absAngle = text->getAngle() + mSymbol.getAngle();
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle < -Angle::deg90() || absAngle >= Angle::deg90());

        // calculate text position
        qreal dx, dy;
        if (text->getAlign().getV() == VAlign::top())
            dy = text->getPosition().toPxQPointF().y()-props.textRect.top();
        else if (text->getAlign().getV() == VAlign::bottom())
            dy = text->getPosition().toPxQPointF().y()-props.textRect.bottom();
        else
            dy = text->getPosition().toPxQPointF().y()-(props.textRect.top()+props.textRect.bottom())/2;
        if (text->getAlign().getH() == HAlign::left())
            dx = text->getPosition().toPxQPointF().x()-props.textRect.left();
        else if (text->getAlign().getH() == HAlign::right())
            dx = text->getPosition().toPxQPointF().x()-props.textRect.right();
        else
            dx = text->getPosition().toPxQPointF().x()-(props.textRect.left()+props.textRect.right())/2;

        // text alignment
        if (props.rotate180)
        {
            props.align = 0;
            if (text->getAlign().getV() == VAlign::top()) props.align |= Qt::AlignBottom;
            if (text->getAlign().getV() == VAlign::center()) props.align |= Qt::AlignVCenter;
            if (text->getAlign().getV() == VAlign::bottom()) props.align |= Qt::AlignTop;
            if (text->getAlign().getH() == HAlign::left()) props.align |= Qt::AlignRight;
            if (text->getAlign().getH() == HAlign::center()) props.align |= Qt::AlignHCenter;
            if (text->getAlign().getH() == HAlign::right()) props.align |= Qt::AlignLeft;
        }
        else
            props.align = text->getAlign().toQtAlign();

        // calculate text bounding rect
        props.textRect = props.textRect.translated(dx, dy).normalized();
        mBoundingRect = mBoundingRect.united(props.textRect);
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

    QPen pen;
    const SchematicLayer* layer = 0;
    const bool selected = mSymbol.isSelected();
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

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(text);
        mFont.setPointSizeF(props.fontSize);

        // draw text or rect
        painter->save();
        if (props.rotate180)
            painter->rotate(text->getAngle().toDeg() + 180);
        else
            painter->rotate(text->getAngle().toDeg());
        if ((deviceIsPrinter) || (lod * text->getHeight().toPx() > 10))
        {
            // draw text
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setFont(mFont);
            painter->drawText(props.textRect, props.align | Qt::TextWordWrap, props.text);
        }
        else
        {
            // fill rect
            painter->fillRect(props.textRect, QBrush(layer->getColor(selected), Qt::Dense5Pattern));
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
