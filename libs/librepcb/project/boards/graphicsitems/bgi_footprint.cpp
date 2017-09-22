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
#include "bgi_footprint.h"
#include "../items/bi_footprint.h"
#include "../board.h"
#include "../../project.h"
#include <librepcb/library/pkg/footprint.h>
#include "../items/bi_device.h"
#include "../boardlayerstack.h"
#include <librepcb/common/attributes/attributesubstitutor.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Footprint::BGI_Footprint(BI_Footprint& footprint) noexcept :
    BGI_Base(), mFootprint(footprint), mLibFootprint(footprint.getLibFootprint())
{
    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

    updateCacheAndRepaint();
}

BGI_Footprint::~BGI_Footprint() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_Footprint::isSelectable() const noexcept
{
    GraphicsLayer* layer = getLayer(GraphicsLayer::sTopReferences);
    return layer && layer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Footprint::updateCacheAndRepaint() noexcept
{
    GraphicsLayer* layer = nullptr;
    prepareGeometryChange();

    mBoundingRect = QRectF();
    mShape = QPainterPath();

    // set Z value
    if (mFootprint.getIsMirrored())
        setZValue(Board::ZValue_FootprintsBottom);
    else
        setZValue(Board::ZValue_FootprintsTop);

    // cross rect
    layer = getLayer(GraphicsLayer::sTopReferences);
    if (layer) {
        if (layer->isVisible()) {
            qreal width = Length(700000).toPx();
            QRectF crossRect(-width, -width, 2*width, 2*width);
            mBoundingRect = mBoundingRect.united(crossRect);
            mShape.addRect(crossRect);
        }
    }

    // polygons
    for (const Polygon& polygon : mLibFootprint.getPolygons()) {
        layer = getLayer(polygon.getLayerName());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        QPainterPath polygonPath = polygon.toQPainterPathPx();
        qreal w = polygon.getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (!polygon.isGrabArea()) continue;
        layer = getLayer(GraphicsLayer::sTopGrabAreas);
        if (!layer) continue;
        if (!layer->isVisible()) continue;
        mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    for (const Text& text : mLibFootprint.getTexts()) {
        layer = getLayer(text.getLayerName());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // create static text properties
        CachedTextProperties_t props;

        // get the text to display
        props.text = AttributeSubstitutor::substitute(text.getText(), &mFootprint);

        // calculate font metrics
        props.fontPixelSize = qCeil(text.getHeight().toPx());
        mFont.setPixelSize(props.fontPixelSize);
        QFontMetricsF metrics(mFont);
        props.scaleFactor = text.getHeight().toPx() / metrics.height();
        props.textRect = metrics.boundingRect(QRectF(), text.getAlign().toQtAlign() |
                                              Qt::TextDontClip, props.text);
        QRectF scaledTextRect = QRectF(props.textRect.topLeft() * props.scaleFactor,
                                       props.textRect.bottomRight() * props.scaleFactor);

        // check rotation
        Angle absAngle = text.getRotation() + mFootprint.getRotation();
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

        // calculate text position
        scaledTextRect.translate(text.getPosition().toPxQPointF());

        // text alignment
        if (props.rotate180)
            props.flags = text.getAlign().mirrored().toQtAlign();
        else
            props.flags = text.getAlign().toQtAlign();

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
        mCachedTextProperties.insert(&text, props);
    }

    if (!mShape.isEmpty())
        mShape.setFillRule(Qt::WindingFill);

    setVisible(!mBoundingRect.isEmpty());

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Footprint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const GraphicsLayer* layer = 0;
    const bool selected = mFootprint.isSelected();
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // draw all polygons
    for (const Polygon& polygon : mLibFootprint.getPolygons()) {
        // get layer
        layer = getLayer(polygon.getLayerName());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // set pen
        if (polygon.getLineWidth() > 0)
            painter->setPen(QPen(layer->getColor(selected), polygon.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);

        // set brush
        if (!polygon.isFilled()) {
            if (polygon.isGrabArea())
                layer = getLayer(GraphicsLayer::sTopGrabAreas);
            else
                layer = nullptr;
        }
        if (layer) {
            if (layer->isVisible())
                painter->setBrush(QBrush(layer->getColor(selected), Qt::SolidPattern));
            else
                painter->setBrush(Qt::NoBrush);
        } else {
            painter->setBrush(Qt::NoBrush);
        }

        // draw polygon
        painter->drawPath(polygon.toQPainterPathPx());
    }

    // draw all ellipses
    for (const Ellipse& ellipse : mLibFootprint.getEllipses()) {
        // get layer
        layer = getLayer(ellipse.getLayerName());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // set pen
        if (ellipse.getLineWidth() > 0)
            painter->setPen(QPen(layer->getColor(selected), ellipse.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        else
            painter->setPen(Qt::NoPen);

        // set brush
        if (!ellipse.isFilled()) {
            if (ellipse.isGrabArea())
                layer = getLayer(GraphicsLayer::sTopGrabAreas);
            else
                layer = nullptr;
        }
        if (layer) {
            if (layer->isVisible())
                painter->setBrush(QBrush(layer->getColor(selected), Qt::SolidPattern));
            else
                painter->setBrush(Qt::NoBrush);
        } else {
            painter->setBrush(Qt::NoBrush);
        }

        // draw ellipse
        painter->drawEllipse(ellipse.getCenter().toPxQPointF(), ellipse.getRadiusX().toPx(),
                             ellipse.getRadiusY().toPx());
        // TODO: rotation
    }

    // draw all texts
    for (const Text& text : mLibFootprint.getTexts()) {
        // get layer
        layer = getLayer(text.getLayerName());
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(&text);
        mFont.setPixelSize(props.fontPixelSize);

        // draw text or rect
        painter->save();
        painter->translate(text.getPosition().toPxQPointF());
        painter->rotate(-text.getRotation().toDeg());
        painter->translate(-text.getPosition().toPxQPointF());
        painter->scale(props.scaleFactor, props.scaleFactor);
        if (props.rotate180) painter->rotate(180);
        if ((deviceIsPrinter) || (lod * text.getHeight().toPx() > 8))
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
        layer = getLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
        if (layer) {
            if (layer->isVisible()) {
                // draw text bounding rect
                painter->setPen(QPen(layer->getColor(selected), 0));
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(props.textRect);
            }
        }
#endif
        painter->restore();
    }

    // draw all holes
    for (const Hole& hole : mLibFootprint.getHoles()) {
        // get layer
        layer = getLayer(GraphicsLayer::sBoardDrillsNpth);
        if (!layer) continue;
        if (!layer->isVisible()) continue;

        // set pen/brush
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(layer->getColor(selected), Qt::SolidPattern));

        // draw hole
        qreal radius = (hole.getDiameter() / 2).toPx();
        painter->drawEllipse(hole.getPosition().toPxQPointF(), radius, radius);
    }

    // draw origin cross
    layer = getLayer(GraphicsLayer::sTopReferences);
    if (layer) {
        if ((!deviceIsPrinter) && layer->isVisible()) {
            qreal width = Length(700000).toPx();
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->drawLine(-width, 0, width, 0);
            painter->drawLine(0, -width, 0, width);
        }
    }

#ifdef QT_DEBUG
    // draw bounding rect
    layer = getLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
    if (layer) {
        if (layer->isVisible()) {
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(mBoundingRect);
        }
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

GraphicsLayer* BGI_Footprint::getLayer(QString name) const noexcept
{
    if (mFootprint.getIsMirrored()) name = GraphicsLayer::getMirroredLayerName(name);
    return mFootprint.getDeviceInstance().getBoard().getLayerStack().getLayer(name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
