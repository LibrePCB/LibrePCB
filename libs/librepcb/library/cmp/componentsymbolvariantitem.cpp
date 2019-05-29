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
#include "componentsymbolvariantitem.h"

#include "component.h"
#include "componentpinsignalmap.h"
#include "componentsymbolvariant.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSymbolVariantItem::ComponentSymbolVariantItem(
    const ComponentSymbolVariantItem& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mSymbolUuid(other.mSymbolUuid),
    mSymbolPos(other.mSymbolPos),
    mSymbolRot(other.mSymbolRot),
    mIsRequired(other.mIsRequired),
    mSuffix(other.mSuffix),
    mPinSignalMap(other.mPinSignalMap),
    mOnPinSignalMapEditedSlot(*this,
                              &ComponentSymbolVariantItem::pinSignalMapEdited) {
  mPinSignalMap.onEdited.attach(mOnPinSignalMapEditedSlot);
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(
    const Uuid& uuid, const Uuid& symbolUuid, const Point& symbolPos,
    const Angle& symbolRotation, bool isRequired,
    const ComponentSymbolVariantItemSuffix& suffix) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mSymbolUuid(symbolUuid),
    mSymbolPos(symbolPos),
    mSymbolRot(symbolRotation),
    mIsRequired(isRequired),
    mSuffix(suffix),
    mOnPinSignalMapEditedSlot(*this,
                              &ComponentSymbolVariantItem::pinSignalMapEdited) {
  mPinSignalMap.onEdited.attach(mOnPinSignalMapEditedSlot);
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const SExpression& node)
  : onEdited(*this),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mSymbolUuid(node.getValueByPath<Uuid>("symbol")),
    mSymbolPos(node.getChildByPath("position")),
    mSymbolRot(node.getValueByPath<Angle>("rotation")),
    mIsRequired(node.getValueByPath<bool>("required")),
    mSuffix(node.getValueByPath<ComponentSymbolVariantItemSuffix>("suffix")),
    mPinSignalMap(node),
    mOnPinSignalMapEditedSlot(*this,
                              &ComponentSymbolVariantItem::pinSignalMapEdited) {
  mPinSignalMap.onEdited.attach(mOnPinSignalMapEditedSlot);
}

ComponentSymbolVariantItem::~ComponentSymbolVariantItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool ComponentSymbolVariantItem::setSymbolUuid(const Uuid& uuid) noexcept {
  if (uuid == mSymbolUuid) {
    return false;
  }

  mSymbolUuid = uuid;
  onEdited.notify(Event::SymbolUuidChanged);
  return true;
}

bool ComponentSymbolVariantItem::setSymbolPosition(const Point& pos) noexcept {
  if (pos == mSymbolPos) {
    return false;
  }

  mSymbolPos = pos;
  onEdited.notify(Event::SymbolPositionChanged);
  return true;
}

bool ComponentSymbolVariantItem::setSymbolRotation(const Angle& rot) noexcept {
  if (rot == mSymbolRot) {
    return false;
  }

  mSymbolRot = rot;
  onEdited.notify(Event::SymbolRotationChanged);
  return true;
}

bool ComponentSymbolVariantItem::setIsRequired(bool required) noexcept {
  if (required == mIsRequired) {
    return false;
  }

  mIsRequired = required;
  onEdited.notify(Event::IsRequiredChanged);
  return true;
}

bool ComponentSymbolVariantItem::setSuffix(
    const ComponentSymbolVariantItemSuffix& suffix) noexcept {
  if (suffix == mSuffix) {
    return false;
  }

  mSuffix = suffix;
  onEdited.notify(Event::SuffixChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSymbolVariantItem::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("symbol", mSymbolUuid, true);
  root.appendChild(mSymbolPos.serializeToDomElement("position"), true);
  root.appendChild("rotation", mSymbolRot, false);
  root.appendChild("required", mIsRequired, false);
  root.appendChild("suffix", mSuffix, false);
  mPinSignalMap.sortedByUuid().serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ComponentSymbolVariantItem::operator==(
    const ComponentSymbolVariantItem& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mSymbolUuid != rhs.mSymbolUuid) return false;
  if (mSymbolPos != rhs.mSymbolPos) return false;
  if (mSymbolRot != rhs.mSymbolRot) return false;
  if (mIsRequired != rhs.mIsRequired) return false;
  if (mSuffix != rhs.mSuffix) return false;
  if (mPinSignalMap != rhs.mPinSignalMap) return false;
  return true;
}

ComponentSymbolVariantItem& ComponentSymbolVariantItem::operator=(
    const ComponentSymbolVariantItem& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setSymbolUuid(rhs.mSymbolUuid);
  setSymbolPosition(rhs.mSymbolPos);
  setSymbolRotation(rhs.mSymbolRot);
  setIsRequired(rhs.mIsRequired);
  setSuffix(rhs.mSuffix);
  mPinSignalMap = rhs.mPinSignalMap;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariantItem::pinSignalMapEdited(
    const ComponentPinSignalMap& map, int index,
    const std::shared_ptr<const ComponentPinSignalMapItem>& item,
    ComponentPinSignalMap::Event                            event) noexcept {
  Q_UNUSED(map);
  Q_UNUSED(index);
  Q_UNUSED(item);
  Q_UNUSED(event);
  onEdited.notify(Event::PinSignalMapEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
