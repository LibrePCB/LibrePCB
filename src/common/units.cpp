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
#include "units.h"
#include "exceptions.h"

/*****************************************************************************************
 *  Class LengthUnit
 ****************************************************************************************/

// Static Variables

LengthUnit::LengthUnit_t LengthUnit::sDefaultUnit = LengthUnit::LengthUnit_t::Millimeters;

// General Methods

QString LengthUnit::toString() const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return QString("millimeters");
        case LengthUnit_t::Micrometers:
            return QString("micrometers");
        case LengthUnit_t::Nanometers:
            return QString("nanometers");
        case LengthUnit_t::Inches:
            return QString("inches");
        case LengthUnit_t::Mils:
            return QString("mils");
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return QString();
    }
}

QString LengthUnit::toStringTr() const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return QCoreApplication::translate("LengthUnit", "Millimeters");
        case LengthUnit_t::Micrometers:
            return QCoreApplication::translate("LengthUnit", "Micrometers");
        case LengthUnit_t::Nanometers:
            return QCoreApplication::translate("LengthUnit", "Nanometers");
        case LengthUnit_t::Inches:
            return QCoreApplication::translate("LengthUnit", "Inches");
        case LengthUnit_t::Mils:
            return QCoreApplication::translate("LengthUnit", "Mils");
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return QString();
    }
}

QString LengthUnit::toShortStringTr() const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return QCoreApplication::translate("LengthUnit", "mm");
        case LengthUnit_t::Micrometers:
            return QCoreApplication::translate("LengthUnit", "μm");
        case LengthUnit_t::Nanometers:
            return QCoreApplication::translate("LengthUnit", "nm");
        case LengthUnit_t::Inches:
            return QCoreApplication::translate("LengthUnit", "″");
        case LengthUnit_t::Mils:
            return QCoreApplication::translate("LengthUnit", "mils");
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return QString();
    }
}

qreal LengthUnit::convertToUnit(const Length& length) const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return length.toMm();
        case LengthUnit_t::Micrometers:
            return length.toMm() * (qreal)1000;
        case LengthUnit_t::Nanometers:
            return (qreal)length.toNm();
        case LengthUnit_t::Inches:
            return length.toInch();
        case LengthUnit_t::Mils:
            return length.toMil();
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return 0;
    }
}

QPointF LengthUnit::convertToUnit(const Point& point) const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return point.toMmQPointF();
        case LengthUnit_t::Micrometers:
            return point.toMmQPointF() * (qreal)1000;
        case LengthUnit_t::Nanometers:
            return point.toMmQPointF() * (qreal)1000000;
        case LengthUnit_t::Inches:
            return point.toInchQPointF();
        case LengthUnit_t::Mils:
            return point.toMilQPointF();
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return QPointF();
    }
}

Length LengthUnit::convertFromUnit(qreal length) const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return Length::fromMm(length);
        case LengthUnit_t::Micrometers:
            return Length::fromMm(length / (qreal)1000);
        case LengthUnit_t::Nanometers:
            return Length::fromMm(length / (qreal)1000000);
        case LengthUnit_t::Inches:
            return Length::fromInch(length);
        case LengthUnit_t::Mils:
            return Length::fromMil(length);
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return Length(0);
    }
}

Point LengthUnit::convertFromUnit(const QPointF& point) const noexcept
{
    switch (mUnit)
    {
        case LengthUnit_t::Millimeters:
            return Point::fromMm(point);
        case LengthUnit_t::Micrometers:
            return Point::fromMm(point / (qreal)1000);
        case LengthUnit_t::Nanometers:
            return Point::fromMm(point / (qreal)1000000);
        case LengthUnit_t::Inches:
            return Point::fromInch(point);
        case LengthUnit_t::Mils:
            return Point::fromMil(point);
        default:
            qCritical() << "invalid length unit:" << static_cast<int>(mUnit);
            Q_ASSERT(false);
            return Point(Length(0), Length(0));
    }
}

// Static Methods

LengthUnit LengthUnit::fromIndex(uint index) throw (Exception)
{
    if (index >= static_cast<uint>(LengthUnit_t::_COUNT))
        throw LogicError(__FILE__, __LINE__, QString::number(index));

    return LengthUnit(static_cast<LengthUnit_t>(index));
}

LengthUnit LengthUnit::fromString(const QString& unitString) throw (Exception)
{
    if (unitString == "millimeters")
        return LengthUnit(LengthUnit_t::Millimeters);
    else if (unitString == "micrometers")
        return LengthUnit(LengthUnit_t::Micrometers);
    else if (unitString == "nanometers")
        return LengthUnit(LengthUnit_t::Nanometers);
    else if (unitString == "inches")
        return LengthUnit(LengthUnit_t::Inches);
    else if (unitString == "mils")
        return LengthUnit(LengthUnit_t::Mils);
    else
    {
        throw RuntimeError(__FILE__, __LINE__, unitString, QString(QCoreApplication::
            translate("LengthUnit", "Invalid length unit: \"%1\"")).arg(unitString));
    }
}

QList<LengthUnit> LengthUnit::getAllUnits() noexcept
{
    QList<LengthUnit> list;
    for (int i = 0; i < static_cast<int>(LengthUnit_t::_COUNT); i++)
        list.append(LengthUnit(static_cast<LengthUnit_t>(i)));
    return list;
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const LengthUnit& unit)
{
    stream << unit.toString();
    return stream;
}

QDebug operator<<(QDebug stream, const LengthUnit& unit)
{
    stream << QString("LengthUnit(%1)").arg(unit.toString());
    return stream;
}

/*****************************************************************************************
 *  Class Length
 ****************************************************************************************/

// General Methods

Length Length::abs() const noexcept
{
    Length l(*this);
    l.makeAbs();
    return l;
}

Length& Length::makeAbs() noexcept
{
    mNanometers = qAbs(mNanometers);
    return *this;
}

Length Length::mappedToGrid(const Length& gridInterval) const noexcept
{
    Length length(*this);
    return length.mapToGrid(gridInterval);
}

Length& Length::mapToGrid(const Length& gridInterval) noexcept
{
    mNanometers = mapNmToGrid(mNanometers, gridInterval);
    return *this;
}

// Static Methods

Length Length::fromMm(qreal millimeters, const Length& gridInterval) throw (RangeError)
{
    Length l;
    l.setLengthMm(millimeters);
    return l.mapToGrid(gridInterval);
}

Length Length::fromMm(const QString& millimeters, const Length& gridInterval) throw (Exception)
{
    Length l;
    l.setLengthMm(millimeters);
    return l.mapToGrid(gridInterval);
}

Length Length::fromInch(qreal inches, const Length& gridInterval) throw (RangeError)
{
    Length l;
    l.setLengthInch(inches);
    return l.mapToGrid(gridInterval);
}

Length Length::fromMil(qreal mils, const Length& gridInterval) throw (RangeError)
{
    Length l;
    l.setLengthMil(mils);
    return l.mapToGrid(gridInterval);
}

Length Length::fromPx(qreal pixels, const Length& gridInterval) throw (RangeError)
{
    Length l;
    l.setLengthPx(pixels);
    return l.mapToGrid(gridInterval);
}

// Private Methods

void Length::setLengthFromFloat(qreal nanometers) throw (RangeError)
{
    LengthBase_t min = std::numeric_limits<LengthBase_t>::min();
    LengthBase_t max = std::numeric_limits<LengthBase_t>::max();
    qreal value = qRound(nanometers);
    if ((value > max) || (value < min))
    {
        throw RangeError(__FILE__, __LINE__, QString("value=%1; min=%2; max=%3")
                         .arg(value).arg(min).arg(max),
                         QCoreApplication::translate("Length", "Range error!"));
    }

    mNanometers = value;
}

// Private Static Methods

LengthBase_t Length::mapNmToGrid(LengthBase_t nanometers, const Length& gridInterval) noexcept
{
    if (gridInterval.mNanometers != 0)
        return qRound((qreal)nanometers / gridInterval.mNanometers) * gridInterval.mNanometers;
    else
        return nanometers;
}

LengthBase_t Length::mmStringToNm(const QString& millimeters) throw (Exception)
{
    bool ok;
    qreal nm = qRound(QLocale::c().toDouble(millimeters, &ok) * 1e6);
    if (!ok)
    {
        throw Exception(__FILE__, __LINE__, millimeters, QString(QCoreApplication::
            translate("Length", "Invalid length string: \"%1\"")).arg(millimeters));
    }
    return nm;
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Length& length)
{
    stream << length.toMm();
    return stream;
}

QDebug operator<<(QDebug stream, const Length& length)
{
    stream << QString("Length(%1mm)").arg(length.toMm());
    return stream;
}

/*****************************************************************************************
 *  Class Angle
 ****************************************************************************************/

// General Methods

Angle Angle::abs() const noexcept
{
    Angle a(*this);
    a.makeAbs();
    return a;
}

Angle& Angle::makeAbs() noexcept
{
    mMicrodegrees = qAbs(mMicrodegrees);
    return *this;
}

Angle Angle::mappedTo0_360deg() const noexcept
{
    Angle a(*this);
    a.mapTo0_360deg();
    return a;
}

Angle& Angle::mapTo0_360deg() noexcept
{
    if (mMicrodegrees < 0)
        mMicrodegrees += 360000000;
    return *this;
}

Angle Angle::mappedTo180deg() const noexcept
{
    Angle a(*this);
    a.mapTo180deg();
    return a;
}

Angle& Angle::mapTo180deg() noexcept
{
    if (mMicrodegrees < -180000000)
        mMicrodegrees += 360000000;
    else if (mMicrodegrees >= 180000000)
        mMicrodegrees -= 360000000;
    return *this;
}

// Static Methods

Angle Angle::fromDeg(qreal degrees) noexcept
{
    Angle angle;
    angle.setAngleDeg(degrees);
    return angle;
}

Angle Angle::fromDeg(const QString& degrees) throw (Exception)
{
    Angle angle;
    angle.setAngleDeg(degrees);
    return angle;
}

Angle Angle::fromRad(qreal radians) noexcept
{
    Angle angle;
    angle.setAngleRad(radians);
    return angle;
}

// Private Static Methods

qint32 Angle::degStringToMicrodeg(const QString& degrees) throw (Exception)
{
    bool ok;
    qreal angle = qRound(QLocale::c().toDouble(degrees, &ok) * 1e6);
    if (!ok)
    {
        throw Exception(__FILE__, __LINE__, degrees, QString(QCoreApplication::
            translate("Angle", "Invalid angle string: \"%1\"")).arg(degrees));
    }
    return angle;
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Angle& angle)
{
    stream << angle.toDeg();
    return stream;
}

QDebug operator<<(QDebug stream, const Angle& angle)
{
    stream << QString("Angle(%1°)").arg(angle.toDeg());
    return stream;
}

/*****************************************************************************************
 *  Class Point
 ****************************************************************************************/

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
        setX(center.getX() + dy);
        setY(center.getY() - dx);
    }
    else if (angle0_360 == Angle::deg180())
    {
        setX(center.getX() - dx);
        setY(center.getY() - dy);
    }
    else if (angle0_360 == Angle::deg270())
    {
        setX(center.getX() - dy);
        setY(center.getY() + dx);
    }
    else if (angle != Angle::deg0())
    {
        // angle is not a multiple of 90 degrees --> we must use floating point arithmetic
        qreal sin = qSin(angle.toRad());
        qreal cos = qCos(angle.toRad());
        setX(Length(center.getX().toNm() + cos * dx.toNm() + sin * dy.toNm()));
        setY(Length(center.getY().toNm() - sin * dx.toNm() + cos * dy.toNm()));
    } // else: angle == 0°, nothing to do...

    return *this;
}

// Static Methods

Point Point::fromMm(qreal millimetersX, qreal millimetersY, const Length& gridInterval) throw (RangeError)
{
    Point p;
    p.mX.setLengthMm(millimetersX);
    p.mY.setLengthMm(millimetersY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromMm(const QPointF& millimeters, const Length& gridInterval) throw (RangeError)
{
    return fromMm(millimeters.x(), millimeters.y(), gridInterval);
}

Point Point::fromInch(qreal inchesX, qreal inchesY, const Length& gridInterval) throw (RangeError)
{
    Point p;
    p.mX.setLengthInch(inchesX);
    p.mY.setLengthInch(inchesY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromInch(const QPointF& inches, const Length& gridInterval) throw (RangeError)
{
    return fromInch(inches.x(), inches.y(), gridInterval);
}

Point Point::fromMil(qreal milsX, qreal milsY, const Length& gridInterval) throw (RangeError)
{
    Point p;
    p.mX.setLengthMil(milsX);
    p.mY.setLengthMil(milsY);
    return p.mapToGrid(gridInterval);
}

Point Point::fromMil(const QPointF& mils, const Length& gridInterval) throw (RangeError)
{
    return fromMil(mils.x(), mils.y(), gridInterval);
}

Point Point::fromPx(qreal pixelsX, qreal pixelsY, const Length& gridInterval) throw (RangeError)
{
    Point p;
    p.mX.setLengthPx(pixelsX);
    p.mY.setLengthPx(-pixelsY); // invert Y!
    return p.mapToGrid(gridInterval);
}

Point Point::fromPx(const QPointF& pixels, const Length& gridInterval) throw (RangeError)
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
