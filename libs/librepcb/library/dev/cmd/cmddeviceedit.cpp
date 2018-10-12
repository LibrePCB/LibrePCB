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
#include "cmddeviceedit.h"

#include "../device.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDeviceEdit::CmdDeviceEdit(Device& device) noexcept
  : UndoCommand(tr("Edit device properties")),
    mDevice(device),
    mOldComponentUuid(device.getComponentUuid()),
    mNewComponentUuid(mOldComponentUuid),
    mOldPackageUuid(device.getPackageUuid()),
    mNewPackageUuid(mOldPackageUuid) {
}

CmdDeviceEdit::~CmdDeviceEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdDeviceEdit::setComponentUuid(const Uuid& uuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewComponentUuid = uuid;
}

void CmdDeviceEdit::setPackageUuid(const Uuid& uuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPackageUuid = uuid;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDeviceEdit::performExecute() {
  performRedo();  // can throw

  if (mNewComponentUuid != mOldComponentUuid) return true;
  if (mNewPackageUuid != mOldPackageUuid) return true;
  return false;
}

void CmdDeviceEdit::performUndo() {
  mDevice.setComponentUuid(mOldComponentUuid);
  mDevice.setPackageUuid(mOldPackageUuid);
}

void CmdDeviceEdit::performRedo() {
  mDevice.setComponentUuid(mNewComponentUuid);
  mDevice.setPackageUuid(mNewPackageUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
