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
#include "cmddeviceinstanceremove.h"
#include "../deviceinstance.h"
#include "../board.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDeviceInstanceRemove::CmdDeviceInstanceRemove(Board& board, DeviceInstance& dev,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove device instance"), parent),
    mBoard(board), mDevice(dev)
{
}

CmdDeviceInstanceRemove::~CmdDeviceInstanceRemove() noexcept
{
    if (isExecuted())
        delete &mDevice;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdDeviceInstanceRemove::redo() throw (Exception)
{
    mBoard.removeDeviceInstance(mDevice); // throws an exception on error

    try
    {
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mBoard.addDeviceInstance(mDevice); // throws an exception on error
        throw;
    }
}

void CmdDeviceInstanceRemove::undo() throw (Exception)
{
    mBoard.addDeviceInstance(mDevice); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception &e)
    {
        mBoard.removeDeviceInstance(mDevice); // throws an exception on error
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
