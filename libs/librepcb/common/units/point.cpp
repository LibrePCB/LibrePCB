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
#include "point.h"
#include "angle.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Point
 ****************************************************************************************/

Point::Point(const SExpression& node)
{
    try {
        mX = node.getChildByIndex(0).getValue<Length>();
        mY = node.getChildByIndex(1).getValue<Length>();
    } catch (const Exception& e) {
        throw FileParseError(__FILE__, __LINE__, node.getFilePath(), -1, -1,
                             QString(), e.getMsg());
    }
}

// General Methods

Point Point::abs() const noexcept
{
    Point p(*this);
    p.makeAbs();
    return p;
}

Point& Point::makeAbs() noexcept
{
    mX.makeAbs();
    mY.makeAbs();
    return *this;
}

Point Point::mappedToGrid(const Length& gridInterval) const noexcept
{
    Point p(*this);
    p.mapToGrid(gridInterval);
    return p;
}

Point& Point::mapToGrid(const Length& gridInterval) noexcept
{
    mX.mapToGrid(gridInterval);
    mY.mapToGrid(gridInterval);
    return *this;
}

Point Point::rotated(const Angle& angle, const Point& center) const noexcept
{
    Point p(*this);
    p.rotate(angle, center);
    return p;
}

Point& Point::rotate(const Angle& angle, const Point& center) noexcept
{
    Length dx = mX - center.getX();
    Length dy = mY - center.getY();
    Angle angle0_360 = angle.mappedTo0_360deg();

    // if angle is a multiple of 90 degrees, rotating can be done without loosing accuracy
    if (angle0_360 == Angle::deg90())
    {
        setX(center.getX() - dy);
        setY(center.getY() + dx);
    }
    else if (angle0_360 == Angle::deg180())
    {
        setX(center.getX() - dx);
        setY(center.getY() - dy);
    }
    else if (angle0_360 == Angle::deg270())
    {
        setX(center.getX() + dy);
        setY(center.getY() - dx);
    }
    else if (angle != Angle::deg0())
    {
        // angle is not a multiple of 90 degrees --> we must use floating point arithmetic
        qreal sin = qSin(angle.toRad());
        qreal cos = qCos(angle.toRad());
        setX(Length(center.getX().toNm() + cos * dx.toNm() - sin * dy.toNm()));
        setY(Length(center.getY().toNm() + sin * dx.toNm() + cos * dy.toNm()));
    } // else: angle == 0°, nothing to do...

    return *this;
}

Point Point::mirrored(Qt::Orientation orientation, const Point& center) const noexcept
{
    Point p(*this);
    p.mirror(orientation, center);
    return p;
}

Point& Point::mirror(Qt::Orientation orientation, const Point& center) noexcept
{
    switch (orientation)
    {
        case Qt::Horizontal:    mX += Length(2) * (center.getX() - mX); break;
        case Qt::Vertical:      mY += Length(2) * (center.getY() - mY); break;
        default: Q_ASSERT(false);
    }
    return *this;
}

void Point::serialize(SExpression& root) const
{
    root.appendChild(mX);
    root.appendChild(mY);
}

// Static Methods

Point Point::fromMm(qreal millimetersX, qreal millimetersY, const Length& gridInterval)
{
    Point p;
    p.mX.setLengthMm(millimetersX);
    p.mY.setLengthMm(millimetersY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromMm(const QPointF& millimeters, const Length& gridInterval)
{
    return fromMm(millimeters.x(), millimeters.y(), gridInterval);
}

Point Point::fromInch(qreal inchesX, qreal inchesY, const Length& gridInterval)
{
    Point p;
    p.mX.setLengthInch(inchesX);
    p.mY.setLengthInch(inchesY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromInch(const QPointF& inches, const Length& gridInterval)
{
    return fromInch(inches.x(), inches.y(), gridInterval);
}

Point Point::fromMil(qreal milsX, qreal milsY, const Length& gridInterval)
{
    Point p;
    p.mX.setLengthMil(milsX);
    p.mY.setLengthMil(milsY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromMil(const QPointF& mils, const Length& gridInterval)
{
    return fromMil(mils.x(), mils.y(), gridInterval);
}

Point Point::fromPx(qreal pixelsX, qreal pixelsY, const Length& gridInterval)
{
    Point p;
    p.mX.setLengthPx(pixelsX);
    p.mY.setLengthPx(-pixelsY); // invert Y!
    return p.mapToGrid(gridInterval);
}

Point Point::fromPx(const QPointF& pixels, const Length& gridInterval)
{
    return fromPx(pixels.x(), pixels.y(), gridInterval);
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Point& point)
{
    stream << point.toMmQPointF();
    return stream;
}

QDebug operator<<(QDebug stream, const Point& point)
{
    stream << QString("Point(%1mm, %2mm)").arg(point.toMmQPointF().x()).arg(point.toMmQPointF().y());
    return stream;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
