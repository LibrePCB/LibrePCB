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
#include "componentpinsignalmap.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentPinSignalMapItem::ComponentPinSignalMapItem(
    const ComponentPinSignalMapItem& other) noexcept
  : onEdited(*this),
    mPinUuid(other.mPinUuid),
    mSignalUuid(other.mSignalUuid),
    mDisplayType(other.mDisplayType) {
}

ComponentPinSignalMapItem::ComponentPinSignalMapItem(
    const Uuid& pin, const std::optional<Uuid>& signal,
    const CmpSigPinDisplayType& displayType) noexcept
  : onEdited(*this),
    mPinUuid(pin),
    mSignalUuid(signal),
    mDisplayType(displayType) {
}

ComponentPinSignalMapItem::ComponentPinSignalMapItem(const SExpression& node)
  : onEdited(*this),
    mPinUuid(deserialize<Uuid>(node.getChild("@0"))),
    mSignalUuid(deserialize<std::optional<Uuid>>(node.getChild("signal/@0"))),
    mDisplayType(
        deserialize<const CmpSigPinDisplayType&>(node.getChild("text/@0"))) {
}

ComponentPinSignalMapItem::~ComponentPinSignalMapItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool ComponentPinSignalMapItem::setSignalUuid(
    const std::optional<Uuid>& uuid) noexcept {
  if (uuid == mSignalUuid) {
    return false;
  }

  mSignalUuid = uuid;
  onEdited.notify(Event::SignalUuidChanged);
  return true;
}
bool ComponentPinSignalMapItem::setDisplayType(
    const CmpSigPinDisplayType& type) noexcept {
  if (type == mDisplayType) {
    return false;
  }

  mDisplayType = type;
  onEdited.notify(Event::DisplayTypeChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentPinSignalMapItem::serialize(SExpression& root) const {
  root.appendChild(mPinUuid);
  root.appendChild("signal", mSignalUuid);
  root.appendChild("text", mDisplayType);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ComponentPinSignalMapItem::operator==(
    const ComponentPinSignalMapItem& rhs) const noexcept {
  if (mPinUuid != rhs.mPinUuid) return false;
  if (mSignalUuid != rhs.mSignalUuid) return false;
  if (mDisplayType != rhs.mDisplayType) return false;
  return true;
}

ComponentPinSignalMapItem& ComponentPinSignalMapItem::operator=(
    const ComponentPinSignalMapItem& rhs) noexcept {
  if (mPinUuid != rhs.mPinUuid) {
    mPinUuid = rhs.mPinUuid;
    onEdited.notify(Event::PinUuidChanged);
  }
  setSignalUuid(rhs.mSignalUuid);
  setDisplayType(rhs.mDisplayType);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
