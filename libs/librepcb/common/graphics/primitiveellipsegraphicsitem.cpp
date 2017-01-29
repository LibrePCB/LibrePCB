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
#include "primitiveellipsegraphicsitem.h"
#include "../toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PrimitiveEllipseGraphicsItem::PrimitiveEllipseGraphicsItem(QGraphicsItem* parent) noexcept :
    QGraphicsItem(parent), mLineLayer(nullptr), mFillLayer(nullptr)
{
    mPen.setWidthF(0);
    mPenHighlighted.setWidthF(0);
    updateColors();
    updateBoundingRectAndShape();
    updateVisibility();
}

PrimitiveEllipseGraphicsItem::~PrimitiveEllipseGraphicsItem() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void PrimitiveEllipseGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitiveEllipseGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void PrimitiveEllipseGraphicsItem::setRadiusX(const Length& rx) noexcept
{
    mEllipseRect.setLeft(-rx.toPx());
    mEllipseRect.setWidth(2*rx.toPx());
    updateBoundingRectAndShape();
}

void PrimitiveEllipseGraphicsItem::setRadiusY(const Length& ry) noexcept
{
    mEllipseRect.setTop(-ry.toPx());
    mEllipseRect.setHeight(2*ry.toPx());
    updateBoundingRectAndShape();
}

void PrimitiveEllipseGraphicsItem::setRadius(const Length& rx, const Length& ry) noexcept
{
    mEllipseRect = Toolbox::boundingRectFromRadius(rx.toPx(), ry.toPx());
    updateBoundingRectAndShape();
}

void PrimitiveEllipseGraphicsItem::setLineWidth(const Length& width) noexcept
{
    mPen.setWidthF(width.toPx());
    mPenHighlighted.setWidthF(width.toPx());
    updateBoundingRectAndShape();
}

void PrimitiveEllipseGraphicsItem::setLineLayer(const GraphicsLayer* layer) noexcept
{
    if (mLineLayer) {
        mLineLayer->unregisterObserver(*this);
    }
    mLineLayer = layer;
    if (mLineLayer) {
        mLineLayer->registerObserver(*this);
    }
    updateColors();
    updateVisibility();
}

void PrimitiveEllipseGraphicsItem::setFillLayer(const GraphicsLayer* layer) noexcept
{
    if (mFillLayer) {
        mFillLayer->unregisterObserver(*this);
    }
    mFillLayer = layer;
    if (mFillLayer) {
        mFillLayer->registerObserver(*this);
    }
    updateColors();
    updateVisibility();
}

/*****************************************************************************************
 *  Inherited from IF_LayerObserver
 ****************************************************************************************/

void PrimitiveEllipseGraphicsItem::layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    if (mLineLayer == &layer) {
        mPen.setColor(newColor);
    }
    if (mFillLayer == &layer) {
        mBrush.setColor(newColor);
    }
    update();
}

void PrimitiveEllipseGraphicsItem::layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    if (mLineLayer == &layer) {
        mPenHighlighted.setColor(newColor);
    }
    if (mFillLayer == &layer) {
        mBrushHighlighted.setColor(newColor);
    }
    update();
}

void PrimitiveEllipseGraphicsItem::layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept
{
    Q_UNUSED(newVisible);
    if (mLineLayer == &layer) {
        mPen.setStyle(layer.isVisible() ? Qt::SolidLine : Qt::NoPen);
        mPenHighlighted.setStyle(layer.isVisible() ? Qt::SolidLine : Qt::NoPen);
    }
    if (mFillLayer == &layer) {
        mBrush.setStyle(layer.isVisible() ? Qt::SolidPattern : Qt::NoBrush);
        mBrushHighlighted.setStyle(layer.isVisible() ? Qt::SolidPattern : Qt::NoBrush);
    }
    updateVisibility();
}

void PrimitiveEllipseGraphicsItem::layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept
{
    layerVisibleChanged(layer, newEnabled);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void PrimitiveEllipseGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);
    if (option->state.testFlag(QStyle::State_Selected)) {
        painter->setPen(mPenHighlighted);
        painter->setBrush(mBrushHighlighted);
    } else {
        painter->setPen(mPen);
        painter->setBrush(mBrush);
    }
    painter->drawEllipse(mEllipseRect);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PrimitiveEllipseGraphicsItem::updateColors() noexcept
{
    if (mLineLayer) {
        mPen.setStyle(Qt::SolidLine);
        mPenHighlighted.setStyle(Qt::SolidLine);
        mPen.setColor(mLineLayer->getColor(false));
        mPenHighlighted.setColor(mLineLayer->getColor(true));
    } else {
        mPen.setStyle(Qt::NoPen);
        mPenHighlighted.setStyle(Qt::NoPen);
    }

    if (mFillLayer) {
        mBrush.setStyle(Qt::SolidPattern);
        mBrushHighlighted.setStyle(Qt::SolidPattern);
        mBrush.setColor(mFillLayer->getColor(false));
        mBrushHighlighted.setColor(mFillLayer->getColor(true));
    } else {
        mBrush.setStyle(Qt::NoBrush);
        mBrushHighlighted.setStyle(Qt::NoBrush);
    }
    update();
}

void PrimitiveEllipseGraphicsItem::updateBoundingRectAndShape() noexcept
{
    prepareGeometryChange();
    mBoundingRect = Toolbox::adjustedBoundingRect(mEllipseRect, mPen.widthF() / 2);
    QPainterPath p;
    p.addEllipse(mEllipseRect);
    mShape = Toolbox::shapeFromPath(p, mPen);
    update();
}

void PrimitiveEllipseGraphicsItem::updateVisibility() noexcept
{
    setVisible((mPen.style() != Qt::NoPen) || (mBrush.style() != Qt::NoBrush));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
