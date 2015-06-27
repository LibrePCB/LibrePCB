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
#include "bgi_footprint.h"
#include "../items/bi_footprint.h"
#include "../board.h"
#include "../../project.h"
#include <librepcbworkspace/workspace.h>
#include <librepcbworkspace/settings/workspacesettings.h>
#include <librepcblibrary/fpt/footprint.h>
#include <librepcbcommon/boardlayer.h>
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Footprint::BGI_Footprint(BI_Footprint& footprint) noexcept :
    BGI_Base(), mFootprint(footprint), mLibFootprint(footprint.getLibFootprint())
{
    setZValue(Board::ZValue_FootprintsTop);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

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
    foreach (const library::FootprintPolygon* polygon, mLibFootprint.getPolygons())
    {
        QPainterPath polygonPath = polygon->toQPainterPathPx();
        qreal w = polygon->getWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    foreach (const library::FootprintText* text, mLibFootprint.getTexts())
    {
        // create static text properties
        CachedTextProperties_t props;

        // get the text to display
        props.text = text->getText();
        mFootprint.replaceVariablesWithAttributes(props.text, true);

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
        Angle absAngle = text->getAngle() + mFootprint.getRotation();
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle < -Angle::deg90() || absAngle >= Angle::deg90());

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

void BGI_Footprint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const BoardLayer* layer = 0;
    const bool selected = mFootprint.isSelected();
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    foreach (const library::FootprintPolygon* polygon, mLibFootprint.getPolygons())
    {
        // set colors
        layer = getBoardLayer(polygon->getLayerId());
        if (layer)
            painter->setPen(QPen(layer->getColor(selected), polygon->getWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);
        if (polygon->isFilled())
            layer = getBoardLayer(polygon->getLayerId());
        else if (polygon->isGrabArea())
            layer = getBoardLayer(BoardLayer::LayerID::FootprintGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon->toQPainterPathPx());
    }

    // draw all ellipses
    foreach (const library::FootprintEllipse* ellipse, mLibFootprint.getEllipses())
    {
        // set colors
        layer = getBoardLayer(ellipse->getLayerId()); if (!layer) continue;
        if (layer)
            painter->setPen(QPen(layer->getColor(selected), ellipse->getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);
        if (ellipse->isFilled())
            layer = getBoardLayer(ellipse->getLayerId());
        else if (ellipse->isGrabArea())
            layer = getBoardLayer(BoardLayer::LayerID::FootprintGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw ellipse
        painter->drawEllipse(ellipse->getCenter().toPxQPointF(), ellipse->getRadiusX().toPx(),
                             ellipse->getRadiusY().toPx());
        // TODO: rotation
    }

    // draw all texts
    foreach (const library::FootprintText* text, mLibFootprint.getTexts())
    {
        // get layer
        layer = getBoardLayer(text->getLayerId()); if (!layer) continue;

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(text);
        mFont.setPixelSize(props.fontPixelSize);

        // draw text or rect
        painter->save();
        painter->scale(props.scaleFactor, props.scaleFactor);
        if (props.rotate180)
            painter->rotate(text->getAngle().toDeg() + 180);
        else
            painter->rotate(text->getAngle().toDeg());
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
    if (mFootprint.getWorkspace().getSettings().getDebugTools()->getShowGraphicsItemsTextBoundingRect())
    {
        // draw text bounding rect
        painter->setPen(QPen(Qt::magenta, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(props.textRect);
    }
#endif
        painter->restore();
    }

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = getBoardLayer(BoardLayer::OriginCrosses);
        if (layer)
        {
            qreal width = Length(700000).toPx();
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->drawLine(-2*width, 0, 2*width, 0);
            painter->drawLine(0, -2*width, 0, 2*width);
        }
    }

#ifdef QT_DEBUG
    if (mFootprint.getWorkspace().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect())
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

BoardLayer* BGI_Footprint::getBoardLayer(uint id) const noexcept
{
    return mFootprint.getComponentInstance().getBoard().getProject().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
