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
#include <librepcb/common/application.h>
#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include "symbolpinpreviewgraphicsitem.h"
#include "../cmp/component.h"
#include <librepcb/common/geometry/text.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPreviewGraphicsItem::SymbolPreviewGraphicsItem(const IF_GraphicsLayerProvider& layerProvider,
                                                     const QStringList& localeOrder,
                                                     const Symbol& symbol,
                                                     const Component* cmp,
                                                     const tl::optional<Uuid>& symbVarUuid,
                                                     const tl::optional<Uuid>& symbVarItemUuid) noexcept :
    QGraphicsItem(), mLayerProvider(layerProvider), mSymbol(symbol), mComponent(cmp),
    mSymbVarItem(nullptr), mDrawBoundingRect(false), mLocaleOrder(localeOrder)
{
    mFont = qApp->getDefaultSansSerifFont();

    try {
        if (mComponent && symbVarUuid && symbVarItemUuid) {
            mSymbVarItem = mComponent->getSymbVarItem(*symbVarUuid, *symbVarItemUuid).get(); // can throw
        }

        updateCacheAndRepaint();

        for (const SymbolPin& pin : symbol.getPins()) {
            const ComponentSignal* signal = nullptr;
            const ComponentPinSignalMapItem* mapItem = nullptr;
            CmpSigPinDisplayType displayType = CmpSigPinDisplayType::pinName();
            if (mComponent && symbVarItemUuid && symbVarItemUuid) {
                signal = mComponent->getSignalOfPin(*symbVarUuid, *symbVarItemUuid, pin.getUuid()).get(); // can throw
            }
            if (mSymbVarItem) mapItem = mSymbVarItem->getPinSignalMap().find(pin.getUuid()).get(); // can throw
            if (mapItem) displayType = mapItem->getDisplayType();
            SymbolPinPreviewGraphicsItem* item = new SymbolPinPreviewGraphicsItem(layerProvider, pin, signal, displayType);
            item->setPos(pin.getPosition().toPxQPointF());
            item->setRotation(-pin.getRotation().toDeg());
            item->setZValue(2);
            item->setParentItem(this);
        }
    } catch (const Exception& e) {
        qCritical() << "Error while loading symbol preview";
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
    for (const Polygon& polygon : mSymbol.getPolygons()) {
        QPainterPath polygonPath = polygon.getPath().toQPainterPathPx();
        qreal w = polygon.getLineWidth().toPx() / 2;
        mBoundingRect = mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
        if (polygon.isGrabArea()) mShape = mShape.united(polygonPath);
    }

    // texts
    mCachedTextProperties.clear();
    for (const Text& text : mSymbol.getTexts()) {
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

void SymbolPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);

    QPen pen;
    const GraphicsLayer* layer = 0;
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    // draw all polygons
    for (const Polygon& polygon : mSymbol.getPolygons()) {
        // set colors
        layer = mLayerProvider.getLayer(polygon.getLayerName());
        if (layer) {
            pen = QPen(layer->getColor(selected), polygon.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        } else {
            painter->setPen(Qt::NoPen);
        }
        if (polygon.isFilled())
            layer = mLayerProvider.getLayer(polygon.getLayerName());
        else if (polygon.isGrabArea())
            layer = mLayerProvider.getLayer(GraphicsLayer::sSymbolGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw polygon
        painter->drawPath(polygon.getPath().toQPainterPathPx());
    }

    // draw all circles
    for (const Circle& circle : mSymbol.getCircles()) {
        // set colors
        layer = mLayerProvider.getLayer(circle.getLayerName()); if (!layer) continue;
        if (layer)
        {
            pen = QPen(layer->getColor(selected), circle.getLineWidth().toPx(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
        }
        else
            painter->setPen(Qt::NoPen);
        if (circle.isFilled())
            layer = mLayerProvider.getLayer(circle.getLayerName());
        else if (circle.isGrabArea())
            layer = mLayerProvider.getLayer(GraphicsLayer::sSymbolGrabAreas);
        else
            layer = nullptr;
        painter->setBrush(layer ? QBrush(layer->getColor(selected), Qt::SolidPattern) : Qt::NoBrush);

        // draw circle
        painter->drawEllipse(circle.getCenter().toPxQPointF(),
                             circle.getDiameter().toPx() / 2,
                             circle.getDiameter().toPx() / 2);
        // TODO: rotation
    }

    // draw all texts
    for (const Text& text : mSymbol.getTexts()) {
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
        layer = mLayerProvider.getLayer(GraphicsLayer::sSchematicReferences);
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

QString SymbolPreviewGraphicsItem::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if (mComponent && mSymbVarItem && (key == QLatin1String("NAME"))) {
        return mComponent->getPrefixes().getDefaultValue() % "?" % mSymbVarItem->getSuffix();
    } else if (mComponent && (key == QLatin1String("NAME"))) {
        return mComponent->getPrefixes().getDefaultValue() % "?";
    } else {
        return "{{ '{{' }}" % key % "{{ '}}' }}";
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
