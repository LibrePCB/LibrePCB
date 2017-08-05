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
#include "lengthunit.h"
#include "length.h"
#include "point.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class LengthUnit
 ****************************************************************************************/

// General Methods

QString LengthUnit::serializeToString() const noexcept
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
            return tr("Millimeters");
        case LengthUnit_t::Micrometers:
            return tr("Micrometers");
        case LengthUnit_t::Nanometers:
            return tr("Nanometers");
        case LengthUnit_t::Inches:
            return tr("Inches");
        case LengthUnit_t::Mils:
            return tr("Mils");
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
            return tr("mm");
        case LengthUnit_t::Micrometers:
            return tr("μm");
        case LengthUnit_t::Nanometers:
            return tr("nm");
        case LengthUnit_t::Inches:
            return tr("″");
        case LengthUnit_t::Mils:
            return tr("mils");
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

LengthUnit LengthUnit::fromIndex(int index) throw (Exception)
{
    if (index >= static_cast<int>(LengthUnit_t::_COUNT))
        throw LogicError(__FILE__, __LINE__, QString::number(index));

    return LengthUnit(static_cast<LengthUnit_t>(index));
}

LengthUnit LengthUnit::deserializeFromString(const QString& str) throw (Exception)
{
    if (str == "millimeters")
        return LengthUnit(LengthUnit_t::Millimeters);
    else if (str == "micrometers")
        return LengthUnit(LengthUnit_t::Micrometers);
    else if (str == "nanometers")
        return LengthUnit(LengthUnit_t::Nanometers);
    else if (str == "inches")
        return LengthUnit(LengthUnit_t::Inches);
    else if (str == "mils")
        return LengthUnit(LengthUnit_t::Mils);
    else
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid length unit: \"%1\"")).arg(str));
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
    stream << unit.serializeToString();
    return stream;
}

QDebug operator<<(QDebug stream, const LengthUnit& unit)
{
    stream << QString("LengthUnit(%1)").arg(unit.serializeToString());
    return stream;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
