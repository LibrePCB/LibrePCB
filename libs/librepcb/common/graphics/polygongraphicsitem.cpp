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
#include "polygongraphicsitem.h"
#include "../graphics/graphicslayer.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PolygonGraphicsItem::PolygonGraphicsItem(Polygon& polygon,
        const IF_GraphicsLayerProvider& lp, QGraphicsItem* parent) noexcept :
    PrimitivePathGraphicsItem(parent), mPolygon(polygon), mLayerProvider(lp)
{
    setPath(mPolygon.toQPainterPathPx());
    setLineWidth(mPolygon.getLineWidth());
    setLineLayer(mLayerProvider.getLayer(mPolygon.getLayerName()));
    updateFillLayer();
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    // register to the polygon to get attribute updates
    mPolygon.registerObserver(*this);
}

PolygonGraphicsItem::~PolygonGraphicsItem() noexcept
{
    mPolygon.unregisterObserver(*this);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PolygonGraphicsItem::polygonLayerNameChanged(const QString& newLayerName) noexcept
{
    setLineLayer(mLayerProvider.getLayer(newLayerName));
    updateFillLayer(); // required if the area is filled with the line layer
}

void PolygonGraphicsItem::polygonLineWidthChanged(const Length& newLineWidth) noexcept
{
    setLineWidth(newLineWidth);
}

void PolygonGraphicsItem::polygonIsFilledChanged(bool newIsFilled) noexcept
{
    Q_UNUSED(newIsFilled);
    updateFillLayer();
}

void PolygonGraphicsItem::polygonIsGrabAreaChanged(bool newIsGrabArea) noexcept
{
    Q_UNUSED(newIsGrabArea);
    updateFillLayer();
}

void PolygonGraphicsItem::polygonStartPosChanged(const Point& newStartPos) noexcept
{
    Q_UNUSED(newStartPos);
    setPath(mPolygon.toQPainterPathPx());
}

void PolygonGraphicsItem::polygonSegmentAdded(int newSegmentIndex) noexcept
{
    Q_UNUSED(newSegmentIndex);
    setPath(mPolygon.toQPainterPathPx());
}

void PolygonGraphicsItem::polygonSegmentRemoved(int oldSegmentIndex) noexcept
{
    Q_UNUSED(oldSegmentIndex);
    setPath(mPolygon.toQPainterPathPx());
}

void PolygonGraphicsItem::polygonSegmentEndPosChanged(int segmentIndex, const Point& newEndPos) noexcept
{
    Q_UNUSED(segmentIndex);
    Q_UNUSED(newEndPos);
    setPath(mPolygon.toQPainterPathPx());
}

void PolygonGraphicsItem::polygonSegmentAngleChanged(int segmentIndex, const Angle& newAngle) noexcept
{
    Q_UNUSED(segmentIndex);
    Q_UNUSED(newAngle);
    setPath(mPolygon.toQPainterPathPx());
}

void PolygonGraphicsItem::updateFillLayer() noexcept
{
    if (mPolygon.isFilled()) {
        setFillLayer(mLayerProvider.getLayer(mPolygon.getLayerName()));
    } else if (mPolygon.isGrabArea()) {
        setFillLayer(mLayerProvider.getGrabAreaLayer(mPolygon.getLayerName()));
    } else {
        setFillLayer(nullptr);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
