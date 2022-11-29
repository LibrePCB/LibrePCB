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
#include "attributetype.h"

#include "attributeunit.h"
#include "attrtypecapacitance.h"
#include "attrtypecurrent.h"
#include "attrtypefrequency.h"
#include "attrtypeinductance.h"
#include "attrtypepower.h"
#include "attrtyperesistance.h"
#include "attrtypestring.h"
#include "attrtypevoltage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttributeType::AttributeType(Type_t type, const QString& typeName,
                             const QString& typeNameTr) noexcept
  : mType(type),
    mTypeName(typeName),
    mTypeNameTr(typeNameTr),
    mDefaultUnit(nullptr) {
}

AttributeType::~AttributeType() noexcept {
  mDefaultUnit = nullptr;
  qDeleteAll(mAvailableUnits);
  mAvailableUnits.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const AttributeUnit* AttributeType::getUnitFromString(
    const QString& unit) const {
  if ((unit.isEmpty() || (unit == "none")) && mAvailableUnits.isEmpty())
    return nullptr;

  foreach (const AttributeUnit* u, mAvailableUnits) {
    if (u->getName() == unit) return u;
  }

  throw RuntimeError(
      __FILE__, __LINE__,
      tr("Unknown unit of attribute type \"%1\": \"%2\"").arg(mTypeName, unit));
}

bool AttributeType::isUnitAvailable(const AttributeUnit* unit) const noexcept {
  if (mAvailableUnits.isEmpty()) {
    return (unit == nullptr);
  } else {
    return mAvailableUnits.contains(unit);
  }
}

const AttributeUnit* AttributeType::tryExtractUnitFromValue(
    QString& value) const noexcept {
  foreach (const AttributeUnit* unit, mAvailableUnits) {
    foreach (const QString& suffix, unit->getUserInputSuffixes()) {
      if (value.endsWith(suffix)) {
        value.chop(suffix.length());
        value = value.trimmed();
        return unit;
      }
    }
  }
  return nullptr;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QList<const AttributeType*> AttributeType::getAllTypes() noexcept {
  QList<const AttributeType*> types;
  types.append(&AttrTypeString::instance());
  types.append(&AttrTypeResistance::instance());
  types.append(&AttrTypeCapacitance::instance());
  types.append(&AttrTypeInductance::instance());
  types.append(&AttrTypeVoltage::instance());
  types.append(&AttrTypeCurrent::instance());
  types.append(&AttrTypePower::instance());
  types.append(&AttrTypeFrequency::instance());
  return types;
}

const AttributeType& AttributeType::fromString(const QString& type) {
  foreach (const AttributeType* t, getAllTypes()) {
    if (t->getName() == type) return *t;
  }
  throw RuntimeError(__FILE__, __LINE__,
                     tr("Invalid attribute type: \"%1\"").arg(type));
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const AttributeType& obj) {
  return SExpression::createToken(obj.getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
