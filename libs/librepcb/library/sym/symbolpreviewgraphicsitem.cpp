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
#include "symbolpreviewgraphicsitem.h"
#include "symbol.h"
#include <librepcbcommon/schematiclayer.h>
#include <librepcbcommon/if_schematiclayerprovider.h>
#include "symbolpinpreviewgraphicsitem.h"
#include "../cmp/component.h"
#include <librepcbcommon/geometry/text.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPreviewGraphicsItem::SymbolPreviewGraphicsItem(const IF_SchematicLayerProvider& layerProvider,
                                                     const QStringList& localeOrder,
                                                     const Symbol& symbol,
                                                     const Component* cmp,
                                                     const Uuid& symbVarUuid,
                                                     const Uuid& symbVarItemUuid) noexcept :
    GraphicsItem(), mLayerProvider(layerProvider), mSymbol(symbol), mComponent(cmp),
    mSymbVarItem(nullptr), mDrawBoundingRect(false), mLocaleOrder(localeOrder)
{
    if (mComponent)
        mSymbVarItem = mComponent->getSymbVarItem(symbVarUuid, symbVarItemUuid);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");

    updateCacheAndRepaint();

    foreach (const Uuid& pinUuid, symbol.getPinUuids())
    {
        const SymbolPin* pin = symbol.getPinByUuid(pinUuid);
        Q_ASSERT(pin); if (!pin) continue;

        const ComponentSignal* signal = nullptr;
        const ComponentPinSignalMapItem* mapItem = nullptr;
        ComponentPinSignalMapItem::PinDisplayType_t displayType =
                ComponentPinSignalMapItem::PinDisplayType_t::PIN_NAME;
        if (mComponent) signal = mComponent->getSignalOfPin(symbVarUuid, symbVarItemUuid, pin->getUuid());
        if (mSymbVarItem) mapItem = mSymbVarItem->getPinSignalMapItemOfPin(pin->getUuid());
        if (mapItem) displayType = mapItem->getDisplayType();
        SymbolPinPreviewGraphicsItem* item = new SymbolPinPreviewGraphicsItem(layerProvider, *pin, signal, displayType);
        item->setPos(pin->getPosition().toPxQPointF());
        item->setRotation(-pin->getRotation().toDeg());
        item->setZValue(2);
        item->setParentItem(this);
    }
}

SymbolPreviewGraphicsItem::~SymbolPreviewGraphicsItem() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SymbolPreviewGraphicsItem::setDrawBoundingRect(bool enable) noexcept
{
    mDrawBoundingRect = enable;
    foreach (QGraphicsItem* child, childItems())
    {
        SymbolPinPreviewGraphicsItem* pin = dynamic_cast<SymbolPinPreviewGraphicsItem*>(child);
        if (pin) pin->setDrawBoundingRect(enable);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPreviewGraphicsItem::updateCacheAndRepaint() noexcept
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
    for (int i = 0; i < mSymbol.getPolygonCount(); i++)
    {
        const Polygon* polygon = mSymbol.getPolygon(i);
        Q_ASSERT(polygon); if (!polygon) continue;

        QPainterPath polygonPath = polygon->toQPainterPathPx();
        qreal w = polygon->getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon->isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    for (int i = 0; i < mSymbol.getTextCount(); i++)
    {
        const Text* text = mSymbol.getText(i);
        Q_ASSERT(text); if (!text) continue;

        // create static text properties
        CachedTextProperties_t props;

        // get the text to display
        props.text = text->getText();
        replaceVariablesWithAttributes(props.text, false);

        // calculate font metrics
        mFont.setPointSizeF(text->getHeight().toPx());
        QFontMetricsF metrics(mFont);
        props.fontSize = text->getHeight().toPx()*0.8*text->getHeight().toPx()/metrics.height();
        mFont.setPointSizeF(props.fontSize);
        metrics = QFontMetricsF(mFont);
        props.textRect = metrics.boundingRect(QRectF(), text->getAlign().toQtAlign() |
                                              Qt::TextDontClip, props.text);

        // check rotation
        Angle absAngle = text->getRotation() + Angle::fromDeg(rotation());
        absAngle.mapTo180deg();
        props.rotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

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

void SymbolPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    QPen pen;
    const SchematicLayer* layer = 0;
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    // draw all polygons
    for (int i = 0; i < mSymbol.getPolygonCount(); i++)
    {
        const Polygon* polygon = mSymbol.getPolygon(i);
        Q_ASSERT(polygon); if (!polygon) continue;

        // set colors
        layer = mLayerProvider.getSchematicLayer(polygon->getLayerId());
        if (layer)
        {
            pen = QPen(layer->getColor(selected), polygon->getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        if (polygon->isFilled())
            layer = mLayerProvider.getSchematicLayer(polygon->getLayerId());
        else if (polygon->isGrabArea())
            layer = mLayerProvider.getSchematicLayer(SchematicLayer::LayerID::SymbolGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon->toQPainterPathPx());
    }

    // draw all ellipses
    for (int i = 0; i < mSymbol.getEllipseCount(); i++)
    {
        const Ellipse* ellipse = mSymbol.getEllipse(i);
        Q_ASSERT(ellipse); if (!ellipse) continue;

        // set colors
        layer = mLayerProvider.getSchematicLayer(ellipse->getLayerId()); if (!layer) continue;
        if (layer)
        {
            pen = QPen(layer->getColor(selected), ellipse->getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        if (ellipse->isFilled())
            layer = mLayerProvider.getSchematicLayer(ellipse->getLayerId());
        else if (ellipse->isGrabArea())
            layer = mLayerProvider.getSchematicLayer(SchematicLayer::LayerID::SymbolGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw ellipse
        painter->drawEllipse(ellipse->getCenter().toPxQPointF(), ellipse->getRadiusX().toPx(),
                             ellipse->getRadiusY().toPx());
        // TODO: rotation
    }

    // draw all texts
    for (int i = 0; i < mSymbol.getTextCount(); i++)
    {
        const Text* text = mSymbol.getText(i);
        Q_ASSERT(text); if (!text) continue;

        // get layer
        layer = mLayerProvider.getSchematicLayer(text->getLayerId()); if (!layer) continue;

        // get cached text properties
        const CachedTextProperties_t& props = mCachedTextProperties.value(text);
        mFont.setPointSizeF(props.fontSize);

        // draw text
        painter->save();
        painter->translate(text->getPosition().toPxQPointF());
        painter->rotate(-text->getRotation().toDeg());
        painter->translate(-text->getPosition().toPxQPointF());
        if (props.rotate180) painter->rotate(180);
        painter->setPen(QPen(layer->getColor(selected), 0));
        painter->setFont(mFont);
        painter->drawText(props.textRect, props.align | Qt::TextWordWrap, props.text);
        painter->restore();
    }

    // draw origin cross
    if (!deviceIsPrinter)
    {
        layer = mLayerProvider.getSchematicLayer(SchematicLayer::OriginCrosses);
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
 *  Private Method
 ****************************************************************************************/

bool SymbolPreviewGraphicsItem::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                                  bool passToParents, QString& value) const noexcept
{
    Q_UNUSED(passToParents);

    if ((attrNS == QLatin1String("SYM")) || (attrNS.isEmpty()))
    {
        if ((attrKey == QLatin1String("NAME")) && (mComponent) && (mSymbVarItem))
            return value = mComponent->getPrefix(mLocaleOrder) % "?" % mSymbVarItem->getSuffix(), true;
    }

    if (((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty())) && (mComponent))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = mComponent->getPrefix(mLocaleOrder) % "?", true;
        if (attrKey == QLatin1String("VALUE"))
            return value = "VALUE", true;
        const LibraryElementAttribute* attr = mComponent->getAttributeByKey(attrKey);
        if (attr) {
            value = attrKey;
            return true;
        }
    }

    if ((attrNS == QLatin1String("PAGE")) || (attrNS.isEmpty()))
    {
        value = attrKey;
        return true;
    }

    value = attrNS % "::" % attrKey;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
