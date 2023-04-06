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
#include "cmddeviceinstanceeditall.h"

#include "cmdboardstroketextedit.h"
#include "cmddeviceinstanceedit.h"

#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDeviceInstanceEditAll::CmdDeviceInstanceEditAll(BI_Device& dev) noexcept
  : UndoCommandGroup(tr("Edit device instance")), mDevEditCmd(nullptr) {
  mDevEditCmd = new CmdDeviceInstanceEdit(dev);
  appendChild(mDevEditCmd);

  foreach (BI_StrokeText* text, dev.getStrokeTexts()) {
    CmdBoardStrokeTextEdit* cmd = new CmdBoardStrokeTextEdit(*text);
    mTextEditCmds.append(cmd);
    appendChild(cmd);
  }
}

CmdDeviceInstanceEditAll::~CmdDeviceInstanceEditAll() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDeviceInstanceEditAll::setPosition(const Point& pos,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  translate(pos - mDevEditCmd->mNewPos, immediate);
}

void CmdDeviceInstanceEditAll::translate(const Point& deltaPos,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mDevEditCmd->translate(deltaPos, immediate);
  foreach (CmdBoardStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->translate(deltaPos, immediate);
  }
}

void CmdDeviceInstanceEditAll::setRotation(const Angle& angle,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  const Angle delta = mDevEditCmd->mNewMirrored
      ? (mDevEditCmd->mNewRotation - angle)  // Mirrored -> inverted rotation!
      : (angle - mDevEditCmd->mNewRotation);
  mDevEditCmd->setRotation(angle, immediate);
  foreach (CmdBoardStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(delta, mDevEditCmd->mNewPos, immediate);
  }
}

void CmdDeviceInstanceEditAll::rotate(const Angle& angle, const Point& center,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mDevEditCmd->rotate(angle, center, immediate);
  foreach (CmdBoardStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(angle, center, immediate);
  }
}

void CmdDeviceInstanceEditAll::setMirrored(bool mirrored, bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  if (mirrored != mDevEditCmd->mNewMirrored) {
    mirror(mDevEditCmd->mNewPos, Qt::Horizontal, immediate);  // can throw
  }
}

void CmdDeviceInstanceEditAll::mirror(const Point& center,
                                      Qt::Orientation orientation,
                                      bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  mDevEditCmd->mirror(center, orientation, immediate);  // can throw
  foreach (CmdBoardStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->mirrorGeometry(orientation, center, immediate);
    cmd->mirrorLayer(immediate);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
