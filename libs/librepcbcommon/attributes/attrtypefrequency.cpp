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
#include "attrtypefrequency.h"
#include "attributeunit.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AttrTypeFrequency::AttrTypeFrequency() noexcept :
    AttributeType(Type_t::Frequency, "frequency", tr("Frequency"))
{
    mDefaultUnit = new AttributeUnit("hertz", tr("Hz"));

    mAvailableUnits.append(new AttributeUnit("microhertz",   tr("μHz")));
    mAvailableUnits.append(new AttributeUnit("millihertz",   tr("mHz")));
    mAvailableUnits.append(mDefaultUnit);
    mAvailableUnits.append(new AttributeUnit("kilohertz",    tr("kHz")));
    mAvailableUnits.append(new AttributeUnit("megahertz",    tr("MHz")));
    mAvailableUnits.append(new AttributeUnit("gigahertz",    tr("GHz")));
}

AttrTypeFrequency::~AttrTypeFrequency() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool AttrTypeFrequency::isValueValid(const QString& value) const noexcept
{
    bool ok = false;
    value.toFloat(&ok);
    return (ok || value.isEmpty());
}

QString AttrTypeFrequency::valueFromTr(const QString& value) const noexcept
{
    bool ok = false;
    float v = QLocale().toFloat(value, &ok);
    if (ok)
        return QString::number(v);
    else
        return QString();
}

QString AttrTypeFrequency::printableValueTr(const QString& value, const AttributeUnit* unit) const noexcept
{
    bool ok = false;
    float v = value.toFloat(&ok);
    if (ok && unit)
        return QString(tr("%1%2")).arg(QLocale().toString(v), unit->getSymbolTr());
    else if (ok)
        return QLocale().toString(v);
    else
        return QString();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
