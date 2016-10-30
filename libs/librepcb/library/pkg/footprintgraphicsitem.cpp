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
#include "footprintgraphicsitem.h"
#include "footprint.h"
#include "footprintpadgraphicsitem.h"
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/graphics/ellipsegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/holegraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintGraphicsItem::FootprintGraphicsItem(Footprint& fpt, const IF_GraphicsLayerProvider& lp) noexcept :
    QGraphicsItem(nullptr), mFootprint(fpt), mLayerProvider(lp)
{
    for (FootprintPad& pad : mFootprint.getPads()) {
        addPad(pad);
    }
    for (Polygon& polygon : mFootprint.getPolygons()) {
        addPolygon(polygon);
    }
    for (Ellipse& ellipse : mFootprint.getEllipses()) {
        addEllipse(ellipse);
    }
    for (Text& text : mFootprint.getTexts()) {
        addText(text);
    }
    for (Hole& hole : mFootprint.getHoles()) {
        addHole(hole);
    }

    // register to the footprint to get attribute updates
    mFootprint.registerGraphicsItem(*this);
}

FootprintGraphicsItem::~FootprintGraphicsItem() noexcept
{
    mFootprint.unregisterGraphicsItem(*this);
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FootprintPadGraphicsItem* FootprintGraphicsItem::getPadGraphicsItem(const FootprintPad& pin) noexcept
{
    return mPadGraphicsItems.value(&pin).data();
}

EllipseGraphicsItem* FootprintGraphicsItem::getEllipseGraphicsItem(const Ellipse& ellipse) noexcept
{
    return mEllipseGraphicsItems.value(&ellipse).data();
}

PolygonGraphicsItem* FootprintGraphicsItem::getPolygonGraphicsItem(const Polygon& polygon) noexcept
{
    return mPolygonGraphicsItems.value(&polygon).data();
}

TextGraphicsItem* FootprintGraphicsItem::getTextGraphicsItem(const Text& text) noexcept
{
    return mTextGraphicsItems.value(&text).data();
}

HoleGraphicsItem* FootprintGraphicsItem::getHoleGraphicsItem(const Hole& hole) noexcept
{
    return mHoleGraphicsItems.value(&hole).data();
}

int FootprintGraphicsItem::getItemsAtPosition(const Point& pos,
    QList<QSharedPointer<FootprintPadGraphicsItem> >* pads,
    QList<QSharedPointer<EllipseGraphicsItem> >* ellipses,
    QList<QSharedPointer<PolygonGraphicsItem> >* polygons,
    QList<QSharedPointer<TextGraphicsItem> >* texts,
    QList<QSharedPointer<HoleGraphicsItem> >* holes) noexcept
{
    int count = 0;
    if (pads) {
        foreach (const QSharedPointer<FootprintPadGraphicsItem>& item, mPadGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                pads->append(item);
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
    if (holes) {
        foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
            QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
            if (item->shape().contains(mappedPos)) {
                holes->append(item);
                ++count;
            }
        }
    }
    return count;
}

QList<QSharedPointer<FootprintPadGraphicsItem> > FootprintGraphicsItem::getSelectedPads() noexcept
{
    QList<QSharedPointer<FootprintPadGraphicsItem>> pins;
    foreach (const QSharedPointer<FootprintPadGraphicsItem>& item, mPadGraphicsItems) {
        if (item->isSelected()) {
            pins.append(item);
        }
    }
    return pins;
}

QList<QSharedPointer<EllipseGraphicsItem> > FootprintGraphicsItem::getSelectedEllipses() noexcept
{
    QList<QSharedPointer<EllipseGraphicsItem>> ellipses;
    foreach (const QSharedPointer<EllipseGraphicsItem>& item, mEllipseGraphicsItems) {
        if (item->isSelected()) {
            ellipses.append(item);
        }
    }
    return ellipses;
}

QList<QSharedPointer<PolygonGraphicsItem> > FootprintGraphicsItem::getSelectedPolygons() noexcept
{
    QList<QSharedPointer<PolygonGraphicsItem>> polygons;
    foreach (const QSharedPointer<PolygonGraphicsItem>& item, mPolygonGraphicsItems) {
        if (item->isSelected()) {
            polygons.append(item);
        }
    }
    return polygons;
}

QList<QSharedPointer<TextGraphicsItem> > FootprintGraphicsItem::getSelectedTexts() noexcept
{
    QList<QSharedPointer<TextGraphicsItem>> texts;
    foreach (const QSharedPointer<TextGraphicsItem>& item, mTextGraphicsItems) {
        if (item->isSelected()) {
            texts.append(item);
        }
    }
    return texts;
}

QList<QSharedPointer<HoleGraphicsItem> > FootprintGraphicsItem::getSelectedHoles() noexcept
{
    QList<QSharedPointer<HoleGraphicsItem>> holes;
    foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
        if (item->isSelected()) {
            holes.append(item);
        }
    }
    return holes;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FootprintGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void FootprintGraphicsItem::addPad(FootprintPad& pad) noexcept
{
    Q_ASSERT(!mPadGraphicsItems.contains(&pad));
    QSharedPointer<FootprintPadGraphicsItem> item(
        new FootprintPadGraphicsItem(pad, mLayerProvider, this));
    mPadGraphicsItems.insert(&pad, item);
}

void FootprintGraphicsItem::removePad(FootprintPad& pad) noexcept
{
    Q_ASSERT(mPadGraphicsItems.contains(&pad));
    mPadGraphicsItems.remove(&pad); // this deletes the graphics item
}

void FootprintGraphicsItem::addEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(!mEllipseGraphicsItems.contains(&ellipse));
    QSharedPointer<EllipseGraphicsItem> item(new EllipseGraphicsItem(ellipse, mLayerProvider, this));
    mEllipseGraphicsItems.insert(&ellipse, item);
}

void FootprintGraphicsItem::removeEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(mEllipseGraphicsItems.contains(&ellipse));
    mEllipseGraphicsItems.remove(&ellipse); // this deletes the graphics item
}

void FootprintGraphicsItem::addPolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(!mPolygonGraphicsItems.contains(&polygon));
    QSharedPointer<PolygonGraphicsItem> item(new PolygonGraphicsItem(polygon, mLayerProvider, this));
    mPolygonGraphicsItems.insert(&polygon, item);
}

void FootprintGraphicsItem::removePolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(mPolygonGraphicsItems.contains(&polygon));
    mPolygonGraphicsItems.remove(&polygon); // this deletes the graphics item
}

void FootprintGraphicsItem::addText(Text& text) noexcept
{
    Q_ASSERT(!mTextGraphicsItems.contains(&text));
    QSharedPointer<TextGraphicsItem> item(new TextGraphicsItem(text, mLayerProvider, this));
    mTextGraphicsItems.insert(&text, item);
}

void FootprintGraphicsItem::removeText(Text& text) noexcept
{
    Q_ASSERT(mTextGraphicsItems.contains(&text));
    mTextGraphicsItems.remove(&text); // this deletes the graphics item
}

void FootprintGraphicsItem::addHole(Hole& hole) noexcept
{
    Q_ASSERT(!mHoleGraphicsItems.contains(&hole));
    QSharedPointer<HoleGraphicsItem> item(new HoleGraphicsItem(hole, mLayerProvider, this));
    mHoleGraphicsItems.insert(&hole, item);
}
void FootprintGraphicsItem::removeHole(Hole& hole) noexcept
{
    Q_ASSERT(mHoleGraphicsItems.contains(&hole));
    mHoleGraphicsItems.remove(&hole); // this deletes the graphics item
}

void FootprintGraphicsItem::setSelectionRect(const QRectF rect) noexcept
{
    QPainterPath path;
    path.addRect(rect);
    foreach (const QSharedPointer<FootprintPadGraphicsItem>& item, mPadGraphicsItems) {
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
    foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
        QPainterPath mappedPath = mapToItem(item.data(), path);
        item->setSelected(item->shape().intersects(mappedPath));
    }
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void FootprintGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
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
