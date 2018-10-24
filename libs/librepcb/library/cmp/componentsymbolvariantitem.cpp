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
  : mUuid(other.mUuid),
    mSymbolUuid(other.mSymbolUuid),
    mSymbolPos(other.mSymbolPos),
    mSymbolRot(other.mSymbolRot),
    mIsRequired(other.mIsRequired),
    mSuffix(other.mSuffix),
    mPinSignalMap(other.mPinSignalMap) {
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(
    const Uuid& uuid, const Uuid& symbolUuid, const Point& symbolPos,
    const Angle& symbolRotation, bool isRequired,
    const ComponentSymbolVariantItemSuffix& suffix) noexcept
  : mUuid(uuid),
    mSymbolUuid(symbolUuid),
    mSymbolPos(symbolPos),
    mSymbolRot(symbolRotation),
    mIsRequired(isRequired),
    mSuffix(suffix) {
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const SExpression& node)
  : mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mSymbolUuid(node.getValueByPath<Uuid>("symbol")),
    mSymbolPos(0, 0),
    mSymbolRot(0),
    mIsRequired(node.getValueByPath<bool>("required")),
    mSuffix(node.getValueByPath<ComponentSymbolVariantItemSuffix>("suffix")),
    mPinSignalMap(node) {
  if (node.tryGetChildByPath("position")) {
    mSymbolPos = Point(node.getChildByPath("position"));
  } else {
    // backward compatibility, remove this some time!
    mSymbolPos = Point(node.getChildByPath("pos"));
  }
  if (node.tryGetChildByPath("rotation")) {
    mSymbolRot = node.getValueByPath<Angle>("rotation");
  } else {
    // backward compatibility, remove this some time!
    mSymbolRot = node.getValueByPath<Angle>("rot");
  }
}

ComponentSymbolVariantItem::~ComponentSymbolVariantItem() noexcept {
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
  mUuid         = rhs.mUuid;
  mSymbolUuid   = rhs.mSymbolUuid;
  mSymbolPos    = rhs.mSymbolPos;
  mSymbolRot    = rhs.mSymbolRot;
  mIsRequired   = rhs.mIsRequired;
  mSuffix       = rhs.mSuffix;
  mPinSignalMap = rhs.mPinSignalMap;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
