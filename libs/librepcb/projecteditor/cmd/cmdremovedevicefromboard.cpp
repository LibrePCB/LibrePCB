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
#include "cmdremovedevicefromboard.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceremove.h>
#include "cmddetachboardnetpointfromviaorpad.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRemoveDeviceFromBoard::CmdRemoveDeviceFromBoard(BI_Device& device) noexcept :
    UndoCommandGroup(tr("Remove device from board")),
    mDevice(device)
{
}

CmdRemoveDeviceFromBoard::~CmdRemoveDeviceFromBoard() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRemoveDeviceFromBoard::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // detach all used netpoints && remove all unused netpoints + netlines
    foreach (BI_FootprintPad* pad, mDevice.getFootprint().getPads()) { Q_ASSERT(pad);
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) { Q_ASSERT(netpoint);
            execNewChildCmd(new CmdDetachBoardNetPointFromViaOrPad(*netpoint)); // can throw
        }
    }

    // remove the device itself
    execNewChildCmd(new CmdDeviceInstanceRemove(mDevice.getBoard(), mDevice)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
