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
    mUuid = QString(); // make UUID invalid
    QString lowercaseUuid = uuid.toLower();
    if (lowercaseUuid.length() != 36)       return false; // do NOT accept '{' and '}'
    QUuid quuid(lowercaseUuid);
    if (quuid.isNull())                     return false;
    if (quuid.variant() != QUuid::DCE)      return false;
    if (quuid.version() != QUuid::Random)   return false;
    mUuid = lowercaseUuid;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Uuid Uuid::createRandom() noexcept
{
    Uuid uuid(QUuid::createUuid().toString().remove("{").remove("}"));
    if (uuid.isNull()) {
        qCritical() << "Could not generate a valid random UUID!";
    }
    return uuid;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
