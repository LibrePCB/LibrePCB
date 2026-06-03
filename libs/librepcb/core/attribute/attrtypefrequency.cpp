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
#include "attrtypefrequency.h"

#include "attributeunit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttrTypeFrequency::AttrTypeFrequency() noexcept
  : AttributeType(Type_t::Frequency, "frequency", tr("Frequency")) {
  mDefaultUnit = new AttributeUnit("hertz", "Hz", {"hz", "Hz"});

  mAvailableUnits.append(
      new AttributeUnit("microhertz", "μHz", {"u", "uhz", "uHz"}));
  mAvailableUnits.append(
      new AttributeUnit("millihertz", "mHz", {"m", "mhz", "mHz"}));
  mAvailableUnits.append(mDefaultUnit);
  mAvailableUnits.append(
      new AttributeUnit("kilohertz", "kHz", {"k", "khz", "kHz"}));
  mAvailableUnits.append(
      new AttributeUnit("megahertz", "MHz", {"M", "meg", "MHz"}));
  mAvailableUnits.append(
      new AttributeUnit("gigahertz", "GHz", {"g", "G", "ghz", "GHz"}));
}

AttrTypeFrequency::~AttrTypeFrequency() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool AttrTypeFrequency::isValueValid(const QString& value) const noexcept {
  bool ok = false;
  value.toFloat(&ok);
  return (ok || value.isEmpty());
}

QString AttrTypeFrequency::valueFromTr(const QString& value) const noexcept {
  bool ok = false;
  float v = QLocale().toFloat(value, &ok);
  if (ok)
    return QString::number(v);
  else
    return QString();
}

QString AttrTypeFrequency::printableValueTr(
    const QString& value, const AttributeUnit* unit) const noexcept {
  // Important: Do not format the number with the user's locale as this would
  // make output files like schematics or even Gerber files non-deterministic.
  // As we know the value is either empty or a valid number in 'C' locale, we
  // can print it as-is, without any further validation or formatting.
  // See: https://github.com/LibrePCB/LibrePCB/issues/1814
  if (unit && (!value.isEmpty())) {
    return value % unit->getSymbolTr();
  } else {
    return value;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
