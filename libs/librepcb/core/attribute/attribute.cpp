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
#include "attribute.h"

#include "attributetype.h"
#include "attributeunit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Attribute::Attribute(const Attribute& other) noexcept
  : onEdited(*this),
    mKey(other.mKey),
    mType(other.mType),
    mValue(other.mValue),
    mUnit(other.mUnit) {
}

Attribute::Attribute(const SExpression& node)
  : onEdited(*this),
    mKey(deserialize<AttributeKey>(node.getChild("@0"))),
    mType(&deserialize<const AttributeType&>(node.getChild("type/@0"))),
    mValue(node.getChild("value/@0").getValue()),
    mUnit(mType->getUnitFromString(node.getChild("unit/@0").getValue())) {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Attribute::Attribute(const AttributeKey& key, const AttributeType& type,
                     const QString& value, const AttributeUnit* unit)
  : onEdited(*this), mKey(key), mType(&type), mValue(value), mUnit(unit) {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Attribute::~Attribute() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString Attribute::getValueTr(bool showUnit) const noexcept {
  return mType->printableValueTr(mValue, showUnit ? mUnit : nullptr);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Attribute::setKey(const AttributeKey& key) noexcept {
  if (key == mKey) {
    return false;
  }

  mKey = key;
  onEdited.notify(Event::KeyChanged);
  return true;
}

bool Attribute::setTypeValueUnit(const AttributeType& type,
                                 const QString& value,
                                 const AttributeUnit* unit) {
  if ((&type == mType) && (value == mValue) && (unit == mUnit)) {
    return false;
  }

  if ((!type.isUnitAvailable(unit)) || (!type.isValueValid(value))) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("%1,%2,%3")
            .arg(type.getName(), value, unit ? unit->getName() : "-"));
  }

  mType = &type;
  mValue = value;
  mUnit = unit;
  onEdited.notify(Event::TypeValueUnitChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Attribute::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
  root.appendChild(mKey);
  root.appendChild("type", *mType);
  if (mUnit) {
    root.appendChild("unit", *mUnit);
  } else {
    root.appendChild("unit", SExpression::createToken("none"));
  }
  root.appendChild("value", mValue);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Attribute::operator==(const Attribute& rhs) const noexcept {
  if (mKey != rhs.mKey) return false;
  if (mType != rhs.mType) return false;
  if (mValue != rhs.mValue) return false;
  if (mUnit != rhs.mUnit) return false;
  return true;
}

Attribute& Attribute::operator=(const Attribute& rhs) noexcept {
  setKey(rhs.mKey);
  setTypeValueUnit(*rhs.mType, rhs.mValue, rhs.mUnit);
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool Attribute::checkAttributesValidity() const noexcept {
  if (!mType->isUnitAvailable(mUnit)) return false;
  if (!mType->isValueValid(mValue)) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
