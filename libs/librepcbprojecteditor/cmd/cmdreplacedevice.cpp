/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdreplacedevice.h"
#include <librepcbproject/project.h>
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/items/bi_device.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceremove.h>
#include "cmdadddevicetoboard.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdReplaceDevice::CmdReplaceDevice(workspace::Workspace& workspace, Board& board,
        BI_Device& device, const Uuid& newDeviceUuid, const Uuid& newFootprintUuid) noexcept :
    UndoCommandGroup(tr("Change Device")), mWorkspace(workspace), mBoard(board),
    mDeviceInstance(device), mNewDeviceUuid(newDeviceUuid),
    mNewFootprintUuid(newFootprintUuid)
{
}

CmdReplaceDevice::~CmdReplaceDevice() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdReplaceDevice::performExecute() throw (Exception)
{
    appendChild(new CmdDeviceInstanceRemove(mBoard, mDeviceInstance));
    appendChild(new CmdAddDeviceToBoard(mWorkspace, mBoard,
                                        mDeviceInstance.getComponentInstance(),
                                        mNewDeviceUuid, mNewFootprintUuid,
                                        mDeviceInstance.getPosition(),
                                        mDeviceInstance.getRotation(),
                                        mDeviceInstance.getIsMirrored()));

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
