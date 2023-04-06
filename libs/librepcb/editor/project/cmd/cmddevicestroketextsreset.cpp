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
#include "cmddevicestroketextsreset.h"

#include "cmddevicestroketextadd.h"
#include "cmddevicestroketextremove.h"

#include <librepcb/core/project/board/items/bi_device.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDeviceStrokeTextsReset::CmdDeviceStrokeTextsReset(BI_Device& device) noexcept
  : UndoCommandGroup(tr("Reset footprint texts")), mDevice(device) {
}

CmdDeviceStrokeTextsReset::~CmdDeviceStrokeTextsReset() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDeviceStrokeTextsReset::performExecute() {
  // Remove all texts
  foreach (BI_StrokeText* text, mDevice.getStrokeTexts()) {
    appendChild(new CmdDeviceStrokeTextRemove(mDevice, *text));
  }

  // Create new texts
  for (const StrokeText& text : mDevice.getDefaultStrokeTexts()) {
    appendChild(new CmdDeviceStrokeTextAdd(
        mDevice,
        *new BI_StrokeText(
            mDevice.getBoard(),
            BoardStrokeTextData(text.getUuid(), text.getLayer(), text.getText(),
                                text.getPosition(), text.getRotation(),
                                text.getHeight(), text.getStrokeWidth(),
                                text.getLetterSpacing(), text.getLineSpacing(),
                                text.getAlign(), text.getMirrored(),
                                text.getAutoRotate(), mDevice.isLocked()))));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
