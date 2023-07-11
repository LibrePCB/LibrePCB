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
#include "devicecheck.h"

#include "device.h"
#include "devicecheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DeviceCheck::DeviceCheck(const Device& device) noexcept
  : LibraryElementCheck(device), mDevice(device) {
}

DeviceCheck::~DeviceCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList DeviceCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkNoPadsConnected(msgs);
  checkParts(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void DeviceCheck::checkNoPadsConnected(MsgList& msgs) const {
  for (const DevicePadSignalMapItem& item : mDevice.getPadSignalMap()) {
    if (item.getSignalUuid()) {
      return;  // pad is connected, don't show this message
    }
  }

  if (!mDevice.getPadSignalMap().isEmpty()) {
    msgs.append(std::make_shared<MsgNoPadsInDeviceConnected>());
  }
}

void DeviceCheck::checkParts(MsgList& msgs) const {
  if (mDevice.getParts().isEmpty()) {
    msgs.append(std::make_shared<MsgDeviceHasNoParts>());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
