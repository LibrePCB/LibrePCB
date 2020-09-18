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
#include "componentsymbolvariant.h"

#include "component.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSymbolVariant::ComponentSymbolVariant(
    const ComponentSymbolVariant& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mNorm(other.mNorm),
    mNames(other.mNames),
    mDescriptions(other.mDescriptions),
    mSymbolItems(other.mSymbolItems),
    mOnItemsEditedSlot(*this, &ComponentSymbolVariant::itemsEdited) {
  mSymbolItems.onEdited.attach(mOnItemsEditedSlot);
}

ComponentSymbolVariant::ComponentSymbolVariant(
    const Uuid& uuid, const QString& norm, const ElementName& name_en_US,
    const QString& desc_en_US) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mNorm(norm),
    mNames(name_en_US),
    mDescriptions(desc_en_US),
    mSymbolItems(),
    mOnItemsEditedSlot(*this, &ComponentSymbolVariant::itemsEdited) {
  mSymbolItems.onEdited.attach(mOnItemsEditedSlot);
}

ComponentSymbolVariant::ComponentSymbolVariant(const SExpression& node)
  : onEdited(*this),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mNorm(node.getValueByPath<QString>("norm")),
    mNames(node),
    mDescriptions(node),
    mSymbolItems(node),
    mOnItemsEditedSlot(*this, &ComponentSymbolVariant::itemsEdited) {
  mSymbolItems.onEdited.attach(mOnItemsEditedSlot);
}

ComponentSymbolVariant::~ComponentSymbolVariant() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool ComponentSymbolVariant::setNorm(const QString& norm) noexcept {
  if (norm == mNorm) {
    return false;
  }

  mNorm = norm;
  onEdited.notify(Event::NormChanged);
  return true;
}

bool ComponentSymbolVariant::setName(const QString& locale,
                                     const ElementName& name) noexcept {
  if (mNames.tryGet(locale) == name) {
    return false;
  }

  mNames.insert(locale, name);
  onEdited.notify(Event::NamesChanged);
  return true;
}

bool ComponentSymbolVariant::setDescription(const QString& locale,
                                            const QString& desc) noexcept {
  if (mDescriptions.tryGet(locale) == desc) {
    return false;
  }

  mDescriptions.insert(locale, desc);
  onEdited.notify(Event::DescriptionsChanged);
  return true;
}

bool ComponentSymbolVariant::setNames(const LocalizedNameMap& names) noexcept {
  if (names == mNames) {
    return false;
  }

  mNames = names;
  onEdited.notify(Event::NamesChanged);
  return true;
}

bool ComponentSymbolVariant::setDescriptions(
    const LocalizedDescriptionMap& descriptions) noexcept {
  if (descriptions == mDescriptions) {
    return false;
  }

  mDescriptions = descriptions;
  onEdited.notify(Event::DescriptionsChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSymbolVariant::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("norm", mNorm, false);
  mNames.serialize(root);
  mDescriptions.serialize(root);
  mSymbolItems.serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ComponentSymbolVariant::operator==(const ComponentSymbolVariant& rhs) const
    noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mNorm != rhs.mNorm) return false;
  if (mNames != rhs.mNames) return false;
  if (mDescriptions != rhs.mDescriptions) return false;
  if (mSymbolItems != rhs.mSymbolItems) return false;
  return true;
}

ComponentSymbolVariant& ComponentSymbolVariant::operator=(
    const ComponentSymbolVariant& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setNorm(rhs.mNorm);
  setNames(rhs.mNames);
  setDescriptions(rhs.mDescriptions);
  mSymbolItems = rhs.mSymbolItems;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariant::itemsEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);
  Q_UNUSED(event);
  onEdited.notify(Event::SymbolItemsEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
