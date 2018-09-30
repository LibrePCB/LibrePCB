/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "cmddevicepadsignalmapitemedit.h"

#include "../devicepadsignalmap.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDevicePadSignalMapItemEdit::CmdDevicePadSignalMapItemEdit(
    DevicePadSignalMapItem& item) noexcept
  : UndoCommand(tr("Edit device pad-signal-map")),
    mItem(item),
    mOldSignalUuid(item.getSignalUuid()),
    mNewSignalUuid(mOldSignalUuid) {
}

CmdDevicePadSignalMapItemEdit::~CmdDevicePadSignalMapItemEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdDevicePadSignalMapItemEdit::setSignalUuid(
    const tl::optional<Uuid>& uuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSignalUuid = uuid;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDevicePadSignalMapItemEdit::performExecute() {
  performRedo();  // can throw

  if (mNewSignalUuid != mOldSignalUuid) return true;
  return false;
}

void CmdDevicePadSignalMapItemEdit::performUndo() {
  mItem.setSignalUuid(mOldSignalUuid);
}

void CmdDevicePadSignalMapItemEdit::performRedo() {
  mItem.setSignalUuid(mNewSignalUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
