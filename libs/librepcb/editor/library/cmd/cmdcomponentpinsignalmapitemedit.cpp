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
#include "cmdcomponentpinsignalmapitemedit.h"

#include <librepcb/core/library/cmp/componentpinsignalmap.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentPinSignalMapItemEdit::CmdComponentPinSignalMapItemEdit(
    const std::shared_ptr<ComponentPinSignalMapItem>& item) noexcept
  : UndoCommand(tr("Edit Component Pinout")),
    mItem(item),
    mOldSignalUuid(item->getSignalUuid()),
    mNewSignalUuid(mOldSignalUuid),
    mOldDisplayType(item->getDisplayType()),
    mNewDisplayType(mOldDisplayType) {
}

CmdComponentPinSignalMapItemEdit::~CmdComponentPinSignalMapItemEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdComponentPinSignalMapItemEdit::setSignalUuid(
    const std::optional<Uuid>& uuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSignalUuid = uuid;
}

void CmdComponentPinSignalMapItemEdit::setDisplayType(
    const CmpSigPinDisplayType& type) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDisplayType = type;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentPinSignalMapItemEdit::performExecute() {
  performRedo();  // can throw

  if (mNewSignalUuid != mOldSignalUuid) return true;
  if (mNewDisplayType != mOldDisplayType) return true;
  return false;
}

void CmdComponentPinSignalMapItemEdit::performUndo() {
  mItem->setSignalUuid(mOldSignalUuid);
  mItem->setDisplayType(mOldDisplayType);
}

void CmdComponentPinSignalMapItemEdit::performRedo() {
  mItem->setSignalUuid(mNewSignalUuid);
  mItem->setDisplayType(mNewDisplayType);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
