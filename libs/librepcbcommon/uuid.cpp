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
#include "uuid.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool Uuid::setUuid(const QString& uuid) noexcept
{
    QUuid quuid(uuid);
    if (uuid.length() != 36)                return false; // do NOT accept '{' and '}'
    if (quuid.isNull())                     return false;
    if (quuid.variant() != QUuid::DCE)      return false;
    if (quuid.version() != QUuid::Random)   return false;
    mUuid = uuid;
    return true;
}

/*****************************************************************************************
 *  Operators
 ****************************************************************************************/

Uuid& Uuid::operator=(const Uuid& rhs) noexcept
{
    mUuid = rhs.mUuid;
    return *this;
}

bool Uuid::operator==(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid == rhs.mUuid);
}

bool Uuid::operator!=(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid != rhs.mUuid);
}

bool Uuid::operator<(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid < rhs.mUuid);
}

bool Uuid::operator>(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid > rhs.mUuid);
}

bool Uuid::operator<=(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid <= rhs.mUuid);
}

bool Uuid::operator>=(const Uuid& rhs) const noexcept
{
    if (mUuid.isEmpty() || rhs.mUuid.isEmpty()) return false;
    return (mUuid >= rhs.mUuid);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Uuid Uuid::createRandom() noexcept
{
    QString str = QUuid::createUuid().toString().remove("{").remove("}");
    Uuid uuid(str);
    Q_ASSERT(uuid.isNull() == false); // TODO: check if this works on windows
    return uuid;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
