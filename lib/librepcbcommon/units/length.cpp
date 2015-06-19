/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <limits>
#include "length.h"

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

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

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

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

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Length::setLengthFromFloat(qreal nanometers) throw (RangeError)
{
    LengthBase_t min = std::numeric_limits<LengthBase_t>::min();
    LengthBase_t max = std::numeric_limits<LengthBase_t>::max();
    qreal value = qRound(nanometers);
    if ((value > max) || (value < min))
    {
        throw RangeError(__FILE__, __LINE__, QString("value=%1; min=%2; max=%3")
            .arg(value).arg(min).arg(max), tr("Range error!"));
    }

    mNanometers = value;
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

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
        throw Exception(__FILE__, __LINE__, millimeters,
            QString(tr("Invalid length string: \"%1\"")).arg(millimeters));
    }
    return nm;
}

/*****************************************************************************************
 *  Non-Member Functions
 ****************************************************************************************/

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
 *  End of File
 ****************************************************************************************/
