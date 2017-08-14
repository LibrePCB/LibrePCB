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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdreplacedevice.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include "cmdadddevicetoboard.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

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

bool CmdReplaceDevice::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // remember which netpoints are connected to which component signal instance
    QList<BI_NetPoint*> attachedNetPoints;
    QHash<BI_NetPoint*, ComponentSignalInstance*> netpointSignalMapping;
    foreach (BI_FootprintPad* pad, mDeviceInstance.getFootprint().getPads()) {
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            if (!attachedNetPoints.contains(netpoint)) {
                attachedNetPoints.append(netpoint);
                netpointSignalMapping.insert(netpoint, pad->getComponentSignalInstance());
            }
        }
    }

    // disconnect all netpoints/netlines
    QList<BI_NetLine*> attachedNetLines;
    foreach (BI_NetPoint* netpoint, attachedNetPoints) {
        foreach (BI_NetLine* netline, netpoint->getLines()) {
            execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
            if (!attachedNetLines.contains(netline)) {
                attachedNetLines.append(netline);
            }
        }
        CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
        cmd->setPadToAttach(nullptr);
        execNewChildCmd(cmd); // can throw
    }

    // replace the device instance
    execNewChildCmd(new CmdDeviceInstanceRemove(mBoard, mDeviceInstance)); // can throw
    CmdAddDeviceToBoard* cmd = new CmdAddDeviceToBoard(mWorkspace, mBoard,
                                                       mDeviceInstance.getComponentInstance(),
                                                       mNewDeviceUuid, mNewFootprintUuid,
                                                       mDeviceInstance.getPosition(),
                                                       mDeviceInstance.getRotation(),
                                                       mDeviceInstance.getIsMirrored());
    execNewChildCmd(cmd); // can throw
    BI_Device* newDevice = cmd->getDeviceInstance();
    Q_ASSERT(newDevice);

    // reconnect all netpoints/netlines
    foreach (BI_NetPoint* netpoint, attachedNetPoints) {
        ComponentSignalInstance* cmpSig = netpointSignalMapping.value(netpoint);
        BI_FootprintPad* newPad = nullptr;
        foreach (BI_FootprintPad* pad, newDevice->getFootprint().getPads()) {
            if (pad->getComponentSignalInstance() == cmpSig) {
                newPad = pad;
                break;
            }
        }
        CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
        cmd->setPadToAttach(newPad);
        execNewChildCmd(cmd); // can throw
    }
    foreach (BI_NetLine* netline, attachedNetLines) {
        execNewChildCmd(new CmdBoardNetLineAdd(*netline)); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
