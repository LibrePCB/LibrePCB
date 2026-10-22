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
#include "cmdnetclassrulesedit.h"

#include <librepcb/core/project/circuit/netclass.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdNetClassRulesEdit::CmdNetClassRulesEdit(NetClass& netclass) noexcept
  : UndoCommand("Edit netclass rules"),
    mNetClass(netclass),
    mOldMinCopperCopperClearance(netclass.getMinCopperCopperClearance()),
    mNewMinCopperCopperClearance(mOldMinCopperCopperClearance),
    mOldMinCopperWidth(netclass.getMinCopperWidth()),
    mNewMinCopperWidth(mOldMinCopperWidth),
    mOldMinViaDrillDiameter(netclass.getMinViaDrillDiameter()),
    mNewMinViaDrillDiameter(mOldMinViaDrillDiameter) {
}

CmdNetClassRulesEdit::~CmdNetClassRulesEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdNetClassRulesEdit::setMinCopperCopperClearance(
    const UnsignedLength& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinCopperCopperClearance = value;
}

void CmdNetClassRulesEdit::setMinCopperWidth(
    const UnsignedLength& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinCopperWidth = value;
}

void CmdNetClassRulesEdit::setMinViaDrillDiameter(
    const UnsignedLength& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinViaDrillDiameter = value;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdNetClassRulesEdit::performExecute() {
  performRedo();  // can throw

  if (mNewMinCopperCopperClearance != mOldMinCopperCopperClearance) return true;
  if (mNewMinCopperWidth != mOldMinCopperWidth) return true;
  if (mNewMinViaDrillDiameter != mOldMinViaDrillDiameter) return true;
  return false;
}

void CmdNetClassRulesEdit::performUndo() {
  mNetClass.setMinCopperCopperClearance(mOldMinCopperCopperClearance);
  mNetClass.setMinCopperWidth(mOldMinCopperWidth);
  mNetClass.setMinViaDrillDiameter(mOldMinViaDrillDiameter);
}

void CmdNetClassRulesEdit::performRedo() {
  mNetClass.setMinCopperCopperClearance(mNewMinCopperCopperClearance);
  mNetClass.setMinCopperWidth(mNewMinCopperWidth);
  mNetClass.setMinViaDrillDiameter(mNewMinViaDrillDiameter);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
