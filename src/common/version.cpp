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
#include "version.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Version::Version() noexcept
{
}

Version::Version(const QString& version) noexcept
{
    setVersion(version);
}

Version::Version(const Version& other) noexcept :
    mVersionStr(other.mVersionStr), mNumbers(other.mNumbers)
{

}

Version::~Version() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool Version::setVersion(const QString& version) noexcept
{
    mVersionStr = version;
    mNumbers.clear();
    QStringList numbers = mVersionStr.split('.', QString::KeepEmptyParts, Qt::CaseSensitive);
    foreach (const QString& number, numbers)
    {
        bool ok = false;
        mNumbers.append(number.toUInt(&ok));
        if (!ok)
        {
            // version invalid --> clear number list and abort
            mNumbers.clear();
            return false;
        }
    }
    return (mNumbers.count() > 0);
}

/*****************************************************************************************
 *  Operators
 ****************************************************************************************/

Version& Version::operator=(const Version& rhs) noexcept
{
    mVersionStr = rhs.mVersionStr;
    mNumbers = rhs.mNumbers;
    return *this;
}

bool Version::operator>(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) > 0);
}

bool Version::operator<(const Version& rhs) const  noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) < 0);
}

bool Version::operator>=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) >= 0);
}

bool Version::operator<=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) <= 0);
}

bool Version::operator==(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) == 0);
}

bool Version::operator!=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty())
    {
        qWarning() << "operators should not be used on invalid Version objects!";
        return false;
    }
    return (compare(rhs) != 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

int Version::compare(const Version& other) const noexcept
{
    if (mNumbers.isEmpty() || other.mNumbers.isEmpty()) return 0;
    if (mNumbers == other.mNumbers) return 0;

    for (int i = 0; i < qMax(mNumbers.count(), other.mNumbers.count()); i++)
    {
        if (i >= mNumbers.count()) return -1;
        if (i >= other.mNumbers.count()) return 1;
        if (mNumbers[i] < other.mNumbers[i]) return -1;
        if (mNumbers[i] > other.mNumbers[i]) return 1;
    }

    qCritical() << "this line should never be reached!";
    return 0;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
