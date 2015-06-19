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
#include "attrtypestring.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AttrTypeString::AttrTypeString() noexcept :
    AttributeType(Type_t::String, "string", tr("String"))
{
}

AttrTypeString::~AttrTypeString() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool AttrTypeString::isValueValid(const QString& value) const noexcept
{
    Q_UNUSED(value);
    return true;
}

QString AttrTypeString::valueFromTr(const QString& value) const noexcept
{
    return value;
}

QString AttrTypeString::printableValueTr(const QString& value, const AttributeUnit* unit) const noexcept
{
    Q_UNUSED(unit);
    return value;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
