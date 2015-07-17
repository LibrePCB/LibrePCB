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
    mNumbers(other.mNumbers)
{

}

Version::~Version() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString Version::toStr() const noexcept
{
    QString str;
    for (int i = 0; i < mNumbers.count(); i++)
    {
        if (i > 0) str.append(".");
        str.append(QString::number(mNumbers.at(i)));
    }
    return str;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool Version::setVersion(const QString& version) noexcept
{
    mNumbers.clear();
    QStringList numbers = version.split('.', QString::KeepEmptyParts, Qt::CaseSensitive);
    foreach (const QString& numberStr, numbers)
    {
        bool ok = false;
        int number = numberStr.toInt(&ok);
        if ((ok) && (number >= 0))
        {
            mNumbers.append(number);
        }
        else
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
    mNumbers = rhs.mNumbers;
    return *this;
}

bool Version::operator>(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty()) return false;
    return (compare(rhs) > 0);
}

bool Version::operator<(const Version& rhs) const  noexcept
{
    if (mNumbers.isEmpty()) return false;
    return (compare(rhs) < 0);
}

bool Version::operator>=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty()) return false;
    return (compare(rhs) >= 0);
}

bool Version::operator<=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty()) return false;
    return (compare(rhs) <= 0);
}

bool Version::operator==(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty()) return false;
    return (compare(rhs) == 0);
}

bool Version::operator!=(const Version& rhs) const noexcept
{
    if (mNumbers.isEmpty()) return false;
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

    Q_ASSERT_X(false, "Version::compare", "this line should never be reached!");
    return 0;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
