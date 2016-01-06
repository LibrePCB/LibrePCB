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
#include "angle.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

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
        throw Exception(__FILE__, __LINE__, degrees,
            QString(tr("Invalid angle string: \"%1\"")).arg(degrees));
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
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
