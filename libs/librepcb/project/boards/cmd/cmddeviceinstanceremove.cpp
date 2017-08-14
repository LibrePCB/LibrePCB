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
#include "cmddeviceinstanceremove.h"
#include "../items/bi_device.h"
#include "../board.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDeviceInstanceRemove::CmdDeviceInstanceRemove(Board& board, BI_Device& dev) noexcept :
    UndoCommand(tr("Remove device instance")),
    mBoard(board), mDevice(dev)
{
}

CmdDeviceInstanceRemove::~CmdDeviceInstanceRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdDeviceInstanceRemove::performExecute()
{
    performRedo(); // can throw

    return true;
}

void CmdDeviceInstanceRemove::performUndo()
{
    mBoard.addDeviceInstance(mDevice); // can throw
}

void CmdDeviceInstanceRemove::performRedo()
{
    mBoard.removeDeviceInstance(mDevice); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
