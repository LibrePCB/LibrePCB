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
#include "symbolgraphicsitem.h"
#include "symbol.h"
#include "symbolpingraphicsitem.h"
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolGraphicsItem::SymbolGraphicsItem(Symbol& symbol, const IF_GraphicsLayerProvider& lp) noexcept :
    QGraphicsItem(nullptr), mSymbol(symbol), mLayerProvider(lp)
{
    for (SymbolPin& pin : mSymbol.getPins()) {
        addPin(pin);
    }
    for (Polygon& polygon : mSymbol.getPolygons()) {
        addPolygon(polygon);
    }
    for (Ellipse& ellipse : mSymbol.getEllipses()) {
        addEllipse(ellipse);
    }
    for (Text& text : mSymbol.getTexts()) {
        addText(text);
    }

    // register to the symbol to get attribute updates
    mSymbol.registerGraphicsItem(*this);
}

SymbolGraphicsItem::~SymbolGraphicsItem() noexcept
{
    mSymbol.unregisterGraphicsItem(*this);
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

SymbolPinGraphicsItem* SymbolGraphicsItem::getPinGraphicsItem(const Uuid& pin) noexcept
{
    return mPinGraphicsItems.value(pin).data();
}

EllipseGraphicsItem* SymbolGraphicsItem::getEllipseGraphicsItem(const Ellipse& ellipse) noexcept
{
    return mEllipseGraphicsItems.value(&ellipse).data();
}

PolygonGraphicsItem* SymbolGraphicsItem::getPolygonGraphicsItem(const Polygon& polygon) noexcept
{
    return mPolygonGraphicsItems.value(&polygon).data();
}

TextGraphicsItem* SymbolGraphicsItem::getTextGraphicsItem(const Text& text) noexcept
{
    return mTextGraphicsItems.value(&text).data();
}

int SymbolGraphicsItem::getItemsAtPosition(const Point& pos,
    QList<QSharedPointer<SymbolPinGraphicsItem>>* pins,
    QList<QSharedPointer<EllipseGraphicsItem> >* ellipses,
    QList<QSharedPointer<PolygonGraphicsItem> >* polygons,
    QList<QSharedPointer<TextGraphicsItem> >* texts) noexcept
{
    int count = 0;
    if (pins) {
        foreach (const QSharedPointer<SymbolPinGraphicsItem>& item, mPinGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                pins->append(item);
                ++count;
            }
        }
    }
    if (ellipses) {
        foreach (const QSharedPointer<EllipseGraphicsItem>& item, mEllipseGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                ellipses->append(item);
                ++count;
            }
        }
    }
    if (polygons) {
        foreach (const QSharedPointer<PolygonGraphicsItem>& item, mPolygonGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                polygons->append(item);
                ++count;
            }
        }
    }
    if (texts) {
        foreach (const QSharedPointer<TextGraphicsItem>& item, mTextGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                texts->append(item);
                ++count;
            }
        }
    }
    return count;
}

QList<QSharedPointer<SymbolPinGraphicsItem>> SymbolGraphicsItem::getSelectedPins() noexcept
{
    QList<QSharedPointer<SymbolPinGraphicsItem>> pins;
    foreach (const QSharedPointer<SymbolPinGraphicsItem>& item, mPinGraphicsItems) {
        if (item->isSelected()) {
            pins.append(item);
        }
    }
    return pins;
}

QList<QSharedPointer<EllipseGraphicsItem> > SymbolGraphicsItem::getSelectedEllipses() noexcept
{
    QList<QSharedPointer<EllipseGraphicsItem>> ellipses;
    foreach (const QSharedPointer<EllipseGraphicsItem>& item, mEllipseGraphicsItems) {
        if (item->isSelected()) {
            ellipses.append(item);
        }
    }
    return ellipses;
}

QList<QSharedPointer<PolygonGraphicsItem> > SymbolGraphicsItem::getSelectedPolygons() noexcept
{
    QList<QSharedPointer<PolygonGraphicsItem>> polygons;
    foreach (const QSharedPointer<PolygonGraphicsItem>& item, mPolygonGraphicsItems) {
        if (item->isSelected()) {
            polygons.append(item);
        }
    }
    return polygons;
}

QList<QSharedPointer<TextGraphicsItem> > SymbolGraphicsItem::getSelectedTexts() noexcept
{
    QList<QSharedPointer<TextGraphicsItem>> texts;
    foreach (const QSharedPointer<TextGraphicsItem>& item, mTextGraphicsItems) {
        if (item->isSelected()) {
            texts.append(item);
        }
    }
    return texts;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void SymbolGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void SymbolGraphicsItem::addPin(SymbolPin& pin) noexcept
{
    Q_ASSERT(!mPinGraphicsItems.contains(pin.getUuid()));
    QSharedPointer<SymbolPinGraphicsItem> item(
                new SymbolPinGraphicsItem(pin, mLayerProvider, this));
    mPinGraphicsItems.insert(pin.getUuid(), item);
}

void SymbolGraphicsItem::removePin(SymbolPin& pin) noexcept
{
    Q_ASSERT(mPinGraphicsItems.contains(pin.getUuid()));
    mPinGraphicsItems.remove(pin.getUuid()); // this deletes the graphics item
}

void SymbolGraphicsItem::addEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(!mEllipseGraphicsItems.contains(&ellipse));
    QSharedPointer<EllipseGraphicsItem> item(new EllipseGraphicsItem(ellipse, mLayerProvider, this));
    mEllipseGraphicsItems.insert(&ellipse, item);
}

void SymbolGraphicsItem::removeEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(mEllipseGraphicsItems.contains(&ellipse));
    mEllipseGraphicsItems.remove(&ellipse); // this deletes the graphics item
}

void SymbolGraphicsItem::addPolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(!mPolygonGraphicsItems.contains(&polygon));
    QSharedPointer<PolygonGraphicsItem> item(new PolygonGraphicsItem(polygon, mLayerProvider, this));
    mPolygonGraphicsItems.insert(&polygon, item);
}

void SymbolGraphicsItem::removePolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(mPolygonGraphicsItems.contains(&polygon));
    mPolygonGraphicsItems.remove(&polygon); // this deletes the graphics item
}

void SymbolGraphicsItem::addText(Text& text) noexcept
{
    Q_ASSERT(!mTextGraphicsItems.contains(&text));
    QSharedPointer<TextGraphicsItem> item(new TextGraphicsItem(text, mLayerProvider, this));
    mTextGraphicsItems.insert(&text, item);
}

void SymbolGraphicsItem::removeText(Text& text) noexcept
{
    Q_ASSERT(mTextGraphicsItems.contains(&text));
    mTextGraphicsItems.remove(&text); // this deletes the graphics item
}

void SymbolGraphicsItem::setSelectionRect(const QRectF rect) noexcept
{
    QPainterPath path;
    path.addRect(rect);
    foreach (const QSharedPointer<SymbolPinGraphicsItem>& item, mPinGraphicsItems) {
        QPainterPath mappedPath = mapToItem(item.data(), path);
        item->setSelected(item->shape().intersects(mappedPath));
    }
    foreach (const QSharedPointer<EllipseGraphicsItem>& item, mEllipseGraphicsItems) {
        QPainterPath mappedPath = mapToItem(item.data(), path);
        item->setSelected(item->shape().intersects(mappedPath));
    }
    foreach (const QSharedPointer<PolygonGraphicsItem>& item, mPolygonGraphicsItems) {
        QPainterPath mappedPath = mapToItem(item.data(), path);
        item->setSelected(item->shape().intersects(mappedPath));
    }
    foreach (const QSharedPointer<TextGraphicsItem>& item, mTextGraphicsItems) {
        QPainterPath mappedPath = mapToItem(item.data(), path);
        item->setSelected(item->shape().intersects(mappedPath));
    }
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SymbolGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
