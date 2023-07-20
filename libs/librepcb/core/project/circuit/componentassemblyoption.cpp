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
#include "componentassemblyoption.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentAssemblyOption::ComponentAssemblyOption(
    const ComponentAssemblyOption& other) noexcept
  : onEdited(*this),
    mDevice(other.mDevice),
    mAttributes(other.mAttributes),
    mAssemblyVariants(other.mAssemblyVariants),
    mParts(other.mParts),
    mOnPartsEditedSlot(*this, &ComponentAssemblyOption::partListEdited) {
  mParts.onEdited.attach(mOnPartsEditedSlot);
}

ComponentAssemblyOption::ComponentAssemblyOption(const SExpression& node)
  : onEdited(*this),
    mDevice(deserialize<Uuid>(node.getChild("@0"))),
    mAttributes(node),
    mAssemblyVariants(),
    mParts(node),
    mOnPartsEditedSlot(*this, &ComponentAssemblyOption::partListEdited) {
  foreach (const SExpression* devNode, node.getChildren("variant")) {
    mAssemblyVariants.insert(deserialize<Uuid>(devNode->getChild("@0")));
  }
  mParts.onEdited.attach(mOnPartsEditedSlot);
}

ComponentAssemblyOption::ComponentAssemblyOption(
    const Uuid& device, const AttributeList& attributes,
    const QSet<Uuid>& assemblyVariants, const PartList& parts)
  : onEdited(*this),
    mDevice(device),
    mAttributes(attributes),
    mAssemblyVariants(assemblyVariants),
    mParts(parts),
    mOnPartsEditedSlot(*this, &ComponentAssemblyOption::partListEdited) {
  mParts.onEdited.attach(mOnPartsEditedSlot);
}

ComponentAssemblyOption::~ComponentAssemblyOption() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentAssemblyOption::setDevice(const Uuid& value) noexcept {
  if (value != mDevice) {
    mDevice = value;
    onEdited.notify(Event::DeviceChanged);
  }
}

void ComponentAssemblyOption::setAttributes(
    const AttributeList& value) noexcept {
  if (value != mAttributes) {
    mAttributes = value;
    onEdited.notify(Event::AttributesChanged);
  }
}

void ComponentAssemblyOption::setAssemblyVariants(
    const QSet<Uuid>& value) noexcept {
  if (value != mAssemblyVariants) {
    mAssemblyVariants = value;
    onEdited.notify(Event::AssemblyVariantsChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentAssemblyOption::serialize(SExpression& root) const {
  root.appendChild(mDevice);
  root.ensureLineBreak();
  mAttributes.serialize(root);
  root.ensureLineBreak();
  foreach (const Uuid& uuid, Toolbox::sortedQSet(mAssemblyVariants)) {
    root.appendChild("variant", uuid);
    root.ensureLineBreak();
  }
  mParts.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ComponentAssemblyOption::operator==(
    const ComponentAssemblyOption& rhs) const noexcept {
  if (mDevice != rhs.mDevice) return false;
  if (mAttributes != rhs.mAttributes) return false;
  if (mAssemblyVariants != rhs.mAssemblyVariants) return false;
  if (mParts != rhs.mParts) return false;
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentAssemblyOption::partListEdited(
    const PartList& list, int index, const std::shared_ptr<const Part>& obj,
    PartList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(obj);
  Q_UNUSED(event);
  onEdited.notify(Event::PartsEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
