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
#include "cmdpartedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPartEdit::CmdPartEdit(const std::shared_ptr<Part>& part) noexcept
  : UndoCommand(tr("Edit Part")),
    mPart(part),
    mOldMpn(part->getMpn()),
    mNewMpn(mOldMpn),
    mOldManufacturer(part->getManufacturer()),
    mNewManufacturer(mOldManufacturer) {
}

CmdPartEdit::~CmdPartEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPartEdit::setMpn(const SimpleString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMpn = value;
}

void CmdPartEdit::setManufacturer(const SimpleString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewManufacturer = value;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPartEdit::performExecute() {
  performRedo();

  if (mNewMpn != mOldMpn) return true;
  if (mNewManufacturer != mOldManufacturer) return true;
  return false;
}

void CmdPartEdit::performUndo() {
  mPart->setMpn(mOldMpn);
  mPart->setManufacturer(mOldManufacturer);
}

void CmdPartEdit::performRedo() {
  mPart->setMpn(mNewMpn);
  mPart->setManufacturer(mNewManufacturer);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
