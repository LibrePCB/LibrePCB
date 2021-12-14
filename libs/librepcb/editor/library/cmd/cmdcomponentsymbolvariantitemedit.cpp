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
#include "cmdcomponentsymbolvariantitemedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentSymbolVariantItemEdit::CmdComponentSymbolVariantItemEdit(
    ComponentSymbolVariantItem& item) noexcept
  : UndoCommand(tr("Edit component symbol variant item")),
    mItem(item),
    mOldSymbolUuid(item.getSymbolUuid()),
    mNewSymbolUuid(mOldSymbolUuid),
    mOldSymbolPos(item.getSymbolPosition()),
    mNewSymbolPos(mOldSymbolPos),
    mOldSymbolRot(item.getSymbolRotation()),
    mNewSymbolRot(mOldSymbolRot),
    mOldIsRequired(item.isRequired()),
    mNewIsRequired(mOldIsRequired),
    mOldSuffix(item.getSuffix()),
    mNewSuffix(mOldSuffix),
    mOldPinSignalMap(item.getPinSignalMap()),
    mNewPinSignalMap(mOldPinSignalMap) {
}

CmdComponentSymbolVariantItemEdit::
    ~CmdComponentSymbolVariantItemEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdComponentSymbolVariantItemEdit::setSymbolUuid(
    const Uuid& uuid) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSymbolUuid = uuid;
}

void CmdComponentSymbolVariantItemEdit::setSymbolPosition(
    const Point& pos) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSymbolPos = pos;
}

void CmdComponentSymbolVariantItemEdit::setSymbolRotation(
    const Angle& rot) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSymbolRot = rot;
}

void CmdComponentSymbolVariantItemEdit::setIsRequired(bool required) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIsRequired = required;
}

void CmdComponentSymbolVariantItemEdit::setSuffix(
    const ComponentSymbolVariantItemSuffix& suffix) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSuffix = suffix;
}

void CmdComponentSymbolVariantItemEdit::setPinSignalMap(
    const ComponentPinSignalMap& map) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPinSignalMap = map;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentSymbolVariantItemEdit::performExecute() {
  performRedo();  // can throw

  if (mNewSymbolUuid != mOldSymbolUuid) return true;
  if (mNewSymbolPos != mOldSymbolPos) return true;
  if (mNewSymbolRot != mOldSymbolRot) return true;
  if (mNewIsRequired != mOldIsRequired) return true;
  if (mNewSuffix != mOldSuffix) return true;
  if (mNewPinSignalMap != mOldPinSignalMap) return true;
  return false;
}

void CmdComponentSymbolVariantItemEdit::performUndo() {
  mItem.setSymbolUuid(mOldSymbolUuid);
  mItem.setSymbolPosition(mOldSymbolPos);
  mItem.setSymbolRotation(mOldSymbolRot);
  mItem.setIsRequired(mOldIsRequired);
  mItem.setSuffix(mOldSuffix);
  mItem.getPinSignalMap() = mOldPinSignalMap;
}

void CmdComponentSymbolVariantItemEdit::performRedo() {
  mItem.setSymbolUuid(mNewSymbolUuid);
  mItem.setSymbolPosition(mNewSymbolPos);
  mItem.setSymbolRotation(mNewSymbolRot);
  mItem.setIsRequired(mNewIsRequired);
  mItem.setSuffix(mNewSuffix);
  mItem.getPinSignalMap() = mNewPinSignalMap;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
