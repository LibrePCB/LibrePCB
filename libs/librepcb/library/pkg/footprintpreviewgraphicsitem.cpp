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
#include "footprintpreviewgraphicsitem.h"
#include "footprint.h"
#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include "footprintpadpreviewgraphicsitem.h"
#include "package.h"
#include <librepcb/common/geometry/text.h>
#include "../cmp/component.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPreviewGraphicsItem::FootprintPreviewGraphicsItem(const IF_GraphicsLayerProvider& layerProvider, const QStringList& localeOrder,
        const Footprint& footprint, const Package* package, /*const Device* device,*/
        const Component* component, const AttributeProvider* attrProvider) noexcept :
    QGraphicsItem(), mLayerProvider(layerProvider), mFootprint(footprint),
    mPackage(package), /*mDevice(device),*/ mComponent(component),
    mAttributeProvider(attrProvider), mDrawBoundingRect(false), mLocaleOrder(localeOrder)
{
    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

    updateCacheAndRepaint();

    for (const FootprintPad& fptPad : footprint.getPads()) {
        const PackagePad* pkgPad = nullptr;
        if (mPackage) pkgPad = mPackage->getPads().find(fptPad.getPackagePadUuid()).get();
        FootprintPadPreviewGraphicsItem* item = new FootprintPadPreviewGraphicsItem(layerProvider, fptPad, pkgPad);
        item->setPos(fptPad.getPosition().toPxQPointF());
        item->setRotation(-fptPad.getRotation().toDeg());
        item->setZValue(-1);
        item->setParentItem(this);
    }
}

FootprintPreviewGraphicsItem::~FootprintPreviewGraphicsItem() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void FootprintPreviewGraphicsItem::setDrawBoundingRect(bool enable) noexcept
{
    mDrawBoundingRect = enable;
    foreach (QGraphicsItem* child, childItems())
    {
        FootprintPadPreviewGraphicsItem* pad = dynamic_cast<FootprintPadPreviewGraphicsItem*>(child);
        if (pad) pad->setDrawBoundingRect(enable);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FootprintPreviewGraphicsItem::updateCacheAndRepaint() noexcept
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
    for (const Polygon& polygon : mFootprint.getPolygons()) {
        QPainterPath polygonPath = polygon.getPath().toQPainterPathPx();
        qreal w = polygon.getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon.isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    for (const Text& text : mFootprint.getTexts()) {
        // create static text properties
        CachedTextProperties_t props;

        // get the text to display
        props.text = AttributeSubstitutor::substitute(text.getText(), this);

        // calculate font metrics
        mFont.setPointSizeF(text.getHeight().toPx());
        QFontMetricsF metrics(mFont);
        props.fontSize = text.getHeight().toPx()*0.8*text.getHeight().toPx()/metrics.height();
        mFont.setPointSizeF(props.fontSize);
        metrics = QFontMetricsF(mFont);
        props.textRect = metrics.boundingRect(QRectF(), text.getAlign().toQtAlign() |
                                              Qt::TextDontClip, props.text);

        // check rotation
        Angle absAngle = text.getRotation() + Angle::fromDeg(rotation());
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

        // calculate text position
        qreal dx, dy;
        if (text.getAlign().getV() == VAlign::top())
            dy = text.getPosition().toPxQPointF().y()-props.textRect.top();
        else if (text.getAlign().getV() == VAlign::bottom())
            dy = text.getPosition().toPxQPointF().y()-props.textRect.bottom();
        else
            dy = text.getPosition().toPxQPointF().y()-(props.textRect.top()+props.textRect.bottom())/2;
        if (text.getAlign().getH() == HAlign::left())
            dx = text.getPosition().toPxQPointF().x()-props.textRect.left();
        else if (text.getAlign().getH() == HAlign::right())
            dx = text.getPosition().toPxQPointF().x()-props.textRect.right();
        else
            dx = text.getPosition().toPxQPointF().x()-(props.textRect.left()+props.textRect.right())/2;

        // text alignment
        if (props.rotate180)
        {
            props.align = 0;
            if (text.getAlign().getV() == VAlign::top()) props.align |= Qt::AlignBottom;
            if (text.getAlign().getV() == VAlign::center()) props.align |= Qt::AlignVCenter;
            if (text.getAlign().getV() == VAlign::bottom()) props.align |= Qt::AlignTop;
            if (text.getAlign().getH() == HAlign::left()) props.align |= Qt::AlignRight;
            if (text.getAlign().getH() == HAlign::center()) props.align |= Qt::AlignHCenter;
            if (text.getAlign().getH() == HAlign::right()) props.align |= Qt::AlignLeft;
        }
        else
            props.align = text.getAlign().toQtAlign();

        // calculate text bounding rect
        props.textRect = props.textRect.translated(dx, dy).normalized();
        mBoundingRect = mBoundingRect.united(props.textRect);
        if (props.rotate180)
        {
            props.textRect = QRectF(-props.textRect.x(), -props.textRect.y(),
                                    -props.textRect.width(), -props.textRect.height()).normalized();
        }

        // save properties
        mCachedTextProperties.insert(&text, props);
    }

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void FootprintPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);

    QPen pen;
    const GraphicsLayer* layer = 0;
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    // draw all polygons
    for (const Polygon& polygon : mFootprint.getPolygons()) {
        // set colors
        layer = mLayerProvider.getLayer(polygon.getLayerName());
        if (layer)
        {
            pen = QPen(layer->getColor(selected), polygon.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        if (polygon.isFilled())
            layer = mLayerProvider.getLayer(polygon.getLayerName());
        else if (polygon.isGrabArea())
            layer = mLayerProvider.getLayer(GraphicsLayer::sTopGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon.getPath().toQPainterPathPx());
    }

    // draw all ellipses
    for (const Ellipse& ellipse : mFootprint.getEllipses()) {
        // set colors
        layer = mLayerProvider.getLayer(ellipse.getLayerName()); if (!layer) continue;
        if (layer)
        {
            pen = QPen(layer->getColor(selected), ellipse.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        if (ellipse.isFilled())
            layer = mLayerProvider.getLayer(ellipse.getLayerName());
        else if (ellipse.isGrabArea())
            layer = mLayerProvider.getLayer(GraphicsLayer::sTopGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw ellipse
        painter->drawEllipse(ellipse.getCenter().toPxQPointF(), ellipse.getRadiusX().toPx(),
                             ellipse.getRadiusY().toPx());
        // TODO: rotation
    }

    // draw all texts
    for (const Text& text : mFootprint.getTexts()) {
        // get layer
        layer = mLayerProvider.getLayer(text.getLayerName()); if (!layer) continue;

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(&text);
        mFont.setPointSizeF(props.fontSize);

        // draw text
        painter->save();
        painter->translate(text.getPosition().toPxQPointF());
        painter->rotate(-text.getRotation().toDeg());
        painter->translate(-text.getPosition().toPxQPointF());
        if (props.rotate180) painter->rotate(180);
        painter->setPen(QPen(layer->getColor(selected), 0));
        painter->setFont(mFont);
        painter->drawText(props.textRect, props.align, props.text);
        painter->restore();
    }

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = mLayerProvider.getLayer(GraphicsLayer::sTopReferences);
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
    if (mDrawBoundingRect)
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  Inherited from AttributeProvider
 ****************************************************************************************/

QString FootprintPreviewGraphicsItem::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if (mAttributeProvider) {
        QString value = mAttributeProvider->getAttributeValue(key);
        if (!value.isEmpty()) return value;
    }
    if (mComponent && (key == QLatin1String("NAME"))) {
        return mComponent->getPrefixes().getDefaultValue() % "?";
    }
    return "##" % key;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
