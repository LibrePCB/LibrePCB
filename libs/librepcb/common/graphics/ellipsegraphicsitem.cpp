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
#include "ellipsegraphicsitem.h"
#include "../graphics/graphicslayer.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

EllipseGraphicsItem::EllipseGraphicsItem(Ellipse& ellipse, const IF_GraphicsLayerProvider& lp,
                                         QGraphicsItem* parent) noexcept :
    PrimitiveEllipseGraphicsItem(parent), mEllipse(ellipse), mLayerProvider(lp)
{
    setPosition(mEllipse.getCenter());
    setRotation(mEllipse.getRotation());
    setRadius(mEllipse.getRadiusX(), mEllipse.getRadiusY());
    setLineWidth(mEllipse.getLineWidth());
    setLineLayer(mLayerProvider.getLayer(mEllipse.getLayerName()));
    updateFillLayer();
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    // register to the ellipse to get attribute updates
    mEllipse.registerObserver(*this);
}

EllipseGraphicsItem::~EllipseGraphicsItem() noexcept
{
    mEllipse.unregisterObserver(*this);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void EllipseGraphicsItem::ellipseLayerNameChanged(const QString& newLayerName) noexcept
{
    setLineLayer(mLayerProvider.getLayer(newLayerName));
    updateFillLayer(); // required if the area is filled with the line layer
}

void EllipseGraphicsItem::ellipseLineWidthChanged(const Length& newLineWidth) noexcept
{
    setLineWidth(newLineWidth);
}

void EllipseGraphicsItem::ellipseIsFilledChanged(bool newIsFilled) noexcept
{
    Q_UNUSED(newIsFilled);
    updateFillLayer();
}

void EllipseGraphicsItem::ellipseIsGrabAreaChanged(bool newIsGrabArea) noexcept
{
    Q_UNUSED(newIsGrabArea);
    updateFillLayer();
}

void EllipseGraphicsItem::ellipseCenterChanged(const Point& newCenter) noexcept
{
    setPosition(newCenter);
}

void EllipseGraphicsItem::ellipseRadiusXChanged(const Length& newRadiusX) noexcept
{
    setRadiusX(newRadiusX);
}

void EllipseGraphicsItem::ellipseRadiusYChanged(const Length& newRadiusY) noexcept
{
    setRadiusY(newRadiusY);
}

void EllipseGraphicsItem::ellipseRotationChanged(const Angle& newRotation) noexcept
{
    setRotation(newRotation);
}

void EllipseGraphicsItem::updateFillLayer() noexcept
{
    if (mEllipse.isFilled()) {
        setFillLayer(mLayerProvider.getLayer(mEllipse.getLayerName()));
    } else if (mEllipse.isGrabArea()) {
        setFillLayer(mLayerProvider.getGrabAreaLayer(mEllipse.getLayerName()));
    } else {
        setFillLayer(nullptr);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
