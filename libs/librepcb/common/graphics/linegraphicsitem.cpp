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
#include "linegraphicsitem.h"
#include "../toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LineGraphicsItem::LineGraphicsItem(QGraphicsItem* parent) noexcept :
    QGraphicsItem(parent), mLayer(nullptr)
{
    mPen.setCapStyle(Qt::RoundCap);
    mPenHighlighted.setCapStyle(Qt::RoundCap);
    mPen.setWidth(0);
    mPenHighlighted.setWidth(0);
    updateBoundingRectAndShape();
    setVisible(false);
}

LineGraphicsItem::~LineGraphicsItem() noexcept
{
    // unregister from graphics layer
    setLayer(nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void LineGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void LineGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void LineGraphicsItem::setLine(const Point& p1, const Point& p2) noexcept
{
    mLine.setPoints(p1.toPxQPointF(), p2.toPxQPointF());
    updateBoundingRectAndShape();
}

void LineGraphicsItem::setLineWidth(const Length& width) noexcept
{
    mPen.setWidthF(width.toPx());
    mPenHighlighted.setWidthF(width.toPx());
    updateBoundingRectAndShape();
}

void LineGraphicsItem::setLayer(const GraphicsLayer* layer) noexcept
{
    if (mLayer) {
        mLayer->unregisterObserver(*this);
    }
    mLayer = layer;
    if (mLayer) {
        mLayer->registerObserver(*this);
        mPen.setColor(mLayer->getColor(false));
        mPenHighlighted.setColor(mLayer->getColor(true));
        setVisible(mLayer->isVisible());
    } else {
        setVisible(false);
    }
}

/*****************************************************************************************
 *  Inherited from IF_LayerObserver
 ****************************************************************************************/

void LineGraphicsItem::layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    Q_UNUSED(layer);
    Q_ASSERT(&layer == mLayer);
    mPen.setColor(newColor);
    update();
}

void LineGraphicsItem::layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    Q_UNUSED(layer);
    Q_ASSERT(&layer == mLayer);
    mPenHighlighted.setColor(newColor);
    update();
}

void LineGraphicsItem::layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept
{
    Q_ASSERT(&layer == mLayer);
    Q_UNUSED(newVisible);
    setVisible(layer.isVisible());
}

void LineGraphicsItem::layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept
{
    Q_ASSERT(&layer == mLayer);
    layerVisibleChanged(layer, newEnabled);
}

void LineGraphicsItem::layerDestroyed(const GraphicsLayer& layer) noexcept
{
    Q_ASSERT(&layer == mLayer);
    setLayer(nullptr);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void LineGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);
    if (option->state.testFlag(QStyle::State_Selected)) {
        painter->setPen(mPenHighlighted);
    } else {
        painter->setPen(mPen);
    }
    painter->drawLine(mLine);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void LineGraphicsItem::updateBoundingRectAndShape() noexcept
{
    prepareGeometryChange();
    QRectF lineRect(mLine.p1(), mLine.p2());
    mBoundingRect = Toolbox::adjustedBoundingRect(lineRect, mPen.widthF() / 2);
    // TODO: Should we also update the shape?
    update();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
