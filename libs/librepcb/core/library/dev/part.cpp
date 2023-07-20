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
#include "part.h"

#include "../../utils/qtmetatyperegistration.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Register meta type.
static QtMetaTypeRegistration<std::shared_ptr<Part>> sSharedMetaType;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Part::Part(const Part& other) noexcept
  : onEdited(*this),
    mMpn(other.mMpn),
    mManufacturer(other.mManufacturer),
    mAttributes(other.mAttributes),
    mOnAttributesEditedSlot(*this, &Part::attributeListEdited) {
  mAttributes.onEdited.attach(mOnAttributesEditedSlot);
}

Part::Part(const SExpression& node)
  : onEdited(*this),
    mMpn(deserialize<SimpleString>(node.getChild("@0"))),
    mManufacturer(deserialize<SimpleString>(node.getChild("manufacturer/@0"))),
    mAttributes(node),
    mOnAttributesEditedSlot(*this, &Part::attributeListEdited) {
  mAttributes.onEdited.attach(mOnAttributesEditedSlot);
}

Part::Part(const SimpleString& mpn, const SimpleString& manufacturer,
           const AttributeList& attributes) noexcept
  : onEdited(*this),
    mMpn(mpn),
    mManufacturer(manufacturer),
    mAttributes(attributes),
    mOnAttributesEditedSlot(*this, &Part::attributeListEdited) {
  mAttributes.onEdited.attach(mOnAttributesEditedSlot);
}

Part::~Part() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Part::isEmpty() const noexcept {
  return mMpn->isEmpty() && mManufacturer->isEmpty() && mAttributes.isEmpty();
}

QStringList Part::getAttributeValuesTr() const noexcept {
  QStringList result;
  for (const Attribute& attribute : mAttributes) {
    const QString value = attribute.getValueTr(true).trimmed();
    if (!value.isEmpty()) {
      result.append(value);
    }
  }
  return result;
}

QStringList Part::getAttributeKeyValuesTr() const noexcept {
  QStringList result;
  for (const Attribute& attribute : mAttributes) {
    result.append(QString("%1=%2")
                      .arg(*attribute.getKey())
                      .arg(attribute.getValueTr(true)));
  }
  return result;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Part::setMpn(const SimpleString& value) noexcept {
  if (value == mMpn) {
    return;
  }

  mMpn = value;
  onEdited.notify(Event::MpnChanged);
}

void Part::setManufacturer(const SimpleString& value) noexcept {
  if (value == mManufacturer) {
    return;
  }

  mManufacturer = value;
  onEdited.notify(Event::ManufacturerChanged);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Part::serialize(SExpression& root) const {
  root.appendChild(mMpn);
  root.appendChild("manufacturer", mManufacturer);
  root.ensureLineBreak();
  mAttributes.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Part::operator==(const Part& rhs) const noexcept {
  if (mMpn != rhs.mMpn) return false;
  if (mManufacturer != rhs.mManufacturer) return false;
  if (mAttributes != rhs.mAttributes) return false;
  return true;
}

Part& Part::operator=(const Part& rhs) noexcept {
  setMpn(rhs.mMpn);
  setManufacturer(rhs.mManufacturer);
  mAttributes = rhs.mAttributes;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Part::attributeListEdited(
    const AttributeList& list, int index,
    const std::shared_ptr<const Attribute>& attribute,
    AttributeList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(attribute);
  Q_UNUSED(event);
  onEdited.notify(Event::AttributesEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
