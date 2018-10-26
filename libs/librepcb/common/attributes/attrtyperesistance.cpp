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
#include "attrtyperesistance.h"

#include "attributeunit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttrTypeResistance::AttrTypeResistance() noexcept
  : AttributeType(Type_t::Resistance, "resistance", tr("Resistance")) {
  mDefaultUnit = new AttributeUnit("ohm", "Ω");

  mAvailableUnits.append(new AttributeUnit("microohm", "μΩ"));
  mAvailableUnits.append(new AttributeUnit("milliohm", "mΩ"));
  mAvailableUnits.append(mDefaultUnit);
  mAvailableUnits.append(new AttributeUnit("kiloohm", "kΩ"));
  mAvailableUnits.append(new AttributeUnit("megaohm", "MΩ"));
}

AttrTypeResistance::~AttrTypeResistance() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool AttrTypeResistance::isValueValid(const QString& value) const noexcept {
  bool ok = false;
  value.toFloat(&ok);
  return (ok || value.isEmpty());
}

QString AttrTypeResistance::valueFromTr(const QString& value) const noexcept {
  bool  ok = false;
  float v  = QLocale().toFloat(value, &ok);
  if (ok)
    return QString::number(v);
  else
    return QString();
}

QString AttrTypeResistance::printableValueTr(const QString&       value,
                                             const AttributeUnit* unit) const
    noexcept {
  bool  ok = false;
  float v  = value.toFloat(&ok);
  if (ok && unit)
    return QString("%1%2").arg(QLocale().toString(v), unit->getSymbolTr());
  else if (ok)
    return QLocale().toString(v);
  else
    return QString();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
