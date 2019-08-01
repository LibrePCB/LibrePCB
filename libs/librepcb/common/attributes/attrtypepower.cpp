/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "attrtypepower.h"

#include "attributeunit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttrTypePower::AttrTypePower() noexcept
  : AttributeType(Type_t::Power, "power", tr("Power")) {
  mDefaultUnit = new AttributeUnit("watt", "W", {"w", "W"});

  mAvailableUnits.append(
      new AttributeUnit("nanowatt", "nW", {"n", "nw", "nW"}));
  mAvailableUnits.append(
      new AttributeUnit("microwatt", "μW", {"u", "uw", "uW"}));
  mAvailableUnits.append(
      new AttributeUnit("milliwatt", "mW", {"m", "mw", "mW"}));
  mAvailableUnits.append(mDefaultUnit);
  mAvailableUnits.append(
      new AttributeUnit("kilowatt", "kW", {"k", "kw", "kW"}));
  mAvailableUnits.append(
      new AttributeUnit("megawatt", "MW", {"M", "meg", "MW"}));
  mAvailableUnits.append(
      new AttributeUnit("gigawatt", "GW", {"g", "G", "gw", "GW"}));
}

AttrTypePower::~AttrTypePower() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool AttrTypePower::isValueValid(const QString& value) const noexcept {
  bool ok = false;
  value.toFloat(&ok);
  return (ok || value.isEmpty());
}

QString AttrTypePower::valueFromTr(const QString& value) const noexcept {
  bool  ok = false;
  float v  = QLocale().toFloat(value, &ok);
  if (ok)
    return QString::number(v);
  else
    return QString();
}

QString AttrTypePower::printableValueTr(const QString&       value,
                                        const AttributeUnit* unit) const
    noexcept {
  bool  ok = false;
  float v  = value.toFloat(&ok);
  if (ok && unit)
    return QLocale().toString(v) % unit->getSymbolTr();
  else if (ok)
    return QLocale().toString(v);
  else
    return QString();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
