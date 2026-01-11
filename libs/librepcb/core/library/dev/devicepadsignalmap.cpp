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
#include "devicepadsignalmap.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DevicePadSignalMapItem::DevicePadSignalMapItem(
    const DevicePadSignalMapItem& other) noexcept
  : onEdited(*this),
    mPadUuid(other.mPadUuid),
    mSignalUuid(other.mSignalUuid),
    mIsOptional(other.mIsOptional) {
}

DevicePadSignalMapItem::DevicePadSignalMapItem(
    const Uuid& pad, const std::optional<Uuid>& signal, bool optional) noexcept
  : onEdited(*this), mPadUuid(pad), mSignalUuid(signal), mIsOptional(optional) {
}

DevicePadSignalMapItem::DevicePadSignalMapItem(const SExpression& node)
  : onEdited(*this),
    mPadUuid(deserialize<Uuid>(node.getChild("@0"))),
    mSignalUuid(deserialize<std::optional<Uuid>>(node.getChild("signal/@0"))),
    mIsOptional(false) {
  // The "optional" attribute was introduced after we started to test
  // LibrePCB 2.0.0-rc1, thus we still allow loading devices without that
  // attribute. Should be changed in a later release.
  if (const SExpression* child = node.tryGetChild("optional/@0")) {
    mIsOptional = deserialize<bool>(*child);
  }
}

DevicePadSignalMapItem::~DevicePadSignalMapItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool DevicePadSignalMapItem::setSignalUuid(
    const std::optional<Uuid>& uuid) noexcept {
  if (uuid == mSignalUuid) {
    return false;
  }

  mSignalUuid = uuid;
  onEdited.notify(Event::SignalUuidChanged);
  return true;
}

bool DevicePadSignalMapItem::setOptional(bool optional) noexcept {
  if (optional == mIsOptional) {
    return false;
  }

  mIsOptional = optional;
  onEdited.notify(Event::IsOptionalChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DevicePadSignalMapItem::serialize(SExpression& root) const {
  root.appendChild(mPadUuid);
  root.appendChild("optional", mIsOptional);
  root.ensureLineBreak();
  root.appendChild("signal", mSignalUuid);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool DevicePadSignalMapItem::operator==(
    const DevicePadSignalMapItem& rhs) const noexcept {
  if (mPadUuid != rhs.mPadUuid) return false;
  if (mSignalUuid != rhs.mSignalUuid) return false;
  if (mIsOptional != rhs.mIsOptional) return false;
  return true;
}

DevicePadSignalMapItem& DevicePadSignalMapItem::operator=(
    const DevicePadSignalMapItem& rhs) noexcept {
  if (rhs.mPadUuid != mPadUuid) {
    mPadUuid = rhs.mPadUuid;
    onEdited.notify(Event::PadUuidChanged);
  }
  setSignalUuid(rhs.mSignalUuid);
  setOptional(rhs.mIsOptional);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
