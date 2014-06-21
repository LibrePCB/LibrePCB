/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <limits>
#include <stdexcept>
#include "units.h"

/*****************************************************************************************
 *  Class Length
 ****************************************************************************************/

// General Methods

Length Length::mappedToGrid(const Length& gridInterval) const
{
    Length length(*this);
    return length.mapToGrid(gridInterval);
}

Length& Length::mapToGrid(const Length& gridInterval)
{
    mNanometers = mapNmToGrid(mNanometers, gridInterval);
    return *this;
}

// Static Methods

Length Length::fromMm(qreal millimeters, const Length& gridInterval)
{
    Length l;
    l.setLengthMm(millimeters);
    return l.mapToGrid(gridInterval);
}

Length Length::fromMm(const QString& millimeters, const Length& gridInterval)
{
    Length l;
    l.setLengthMm(millimeters);
    return l.mapToGrid(gridInterval);
}

Length Length::fromInch(qreal inches, const Length& gridInterval)
{
    Length l;
    l.setLengthInch(inches);
    return l.mapToGrid(gridInterval);
}

Length Length::fromMil(qreal mils, const Length& gridInterval)
{
    Length l;
    l.setLengthMil(mils);
    return l.mapToGrid(gridInterval);
}

Length Length::fromPx(qreal pixels, const Length& gridInterval)
{
    Length l;
    l.setLengthPx(pixels);
    return l.mapToGrid(gridInterval);
}

// Private Methods

void Length::setLengthFromFloat(qreal nanometers)
{
    if ((nanometers > std::numeric_limits<LengthBase_t>::max())
            || (nanometers < std::numeric_limits<LengthBase_t>::min()))
        throw std::range_error("Range error in the Length class!");

    mNanometers = nanometers;
}

// Private Static Methods

/**
 * @todo    does this work correctly with large 64bit integers?!
 *          and maybe there is a better, integer-based method for this purpose?
 */
LengthBase_t Length::mapNmToGrid(LengthBase_t nanometers, const Length& gridInterval)
{
    if (gridInterval.mNanometers != 0)
        return qRound((qreal)nanometers / gridInterval.mNanometers) * gridInterval.mNanometers;
    else
        return nanometers;
}

/**
 * @todo    make this function better...this does not work for 32bit AND 64bit integers!
 *          and throw an exception if an range error occurs (under-/overflow)!
 */
LengthBase_t Length::mmStringToNm(const QString& millimeters)
{
    return millimeters.toInt();
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Length& length)
{
    stream << length.toMm();
    return stream;
}

QDebug& operator<<(QDebug& stream, const Length& length)
{
    stream << QString("Length(%1mm)").arg(length.toMm());
    return stream;
}

/*****************************************************************************************
 *  Class Angle
 ****************************************************************************************/

// Static Methods

Angle Angle::fromDeg(qreal degrees)
{
    Angle angle;
    angle.setAngleDeg(degrees);
    return angle;
}

Angle Angle::fromDeg(const QString& degrees)
{
    Angle angle;
    angle.setAngleDeg(degrees);
    return angle;
}

Angle Angle::fromRad(qreal radians)
{
    Angle angle;
    angle.setAngleRad(radians);
    return angle;
}

// Private Static Methods

qint32 Angle::degStringToMicrodeg(const QString& degrees)
{
    return fmod(degrees.toDouble(), 360e6); ///< @todo fmod is only for double, so not good for ARM!
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Angle& angle)
{
    stream << angle.toDeg();
    return stream;
}

QDebug& operator<<(QDebug& stream, const Angle& angle)
{
    stream << QString("Angle(%1°)").arg(angle.toDeg());
    return stream;
}

/*****************************************************************************************
 *  Class Point
 ****************************************************************************************/

// Setters

void Point::setXY(const QString& point, const QChar& sep)
{
    QStringList coordinates = point.split(sep, QString::SkipEmptyParts);
    if (coordinates.count() != 2)
        throw std::runtime_error("Invalid Point String!");

    mX.setLengthMm(coordinates[0]);
    mY.setLengthMm(coordinates[1]);
}

// General Methods

Point Point::mappedToGrid(const Length& gridInterval) const
{
    Point p(*this);
    p.mapToGrid(gridInterval);
    return p;
}

Point& Point::mapToGrid(const Length& gridInterval)
{
    mX.mapToGrid(gridInterval);
    mY.mapToGrid(gridInterval);
    return *this;
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
    p.mY.setLengthPx(pixelsY);
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

QDebug& operator<<(QDebug& stream, const Point& point)
{
    stream << QString("Point(%1mm, %2mm)").arg(point.toMmQPointF().x()).arg(point.toMmQPointF().y());
    return stream;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
