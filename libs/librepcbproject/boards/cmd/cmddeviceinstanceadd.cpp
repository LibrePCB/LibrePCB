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
#include "cmddeviceinstanceadd.h"
#include "../deviceinstance.h"
#include "../board.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDeviceInstanceAdd::CmdDeviceInstanceAdd(Board& board, ComponentInstance& comp,
                                           const QUuid& deviceUuid,
                                           const Point& position, const Angle& rotation,
                                           UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add device to board"), parent),
    mBoard(board), mDeviceInstance(nullptr)
{
    mDeviceInstance = new DeviceInstance(board, comp, deviceUuid, position, rotation);
}

CmdDeviceInstanceAdd::CmdDeviceInstanceAdd(DeviceInstance& device,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add device to board"), parent),
    mBoard(device.getBoard()), mDeviceInstance(&device)
{
}

CmdDeviceInstanceAdd::~CmdDeviceInstanceAdd() noexcept
{
    if (!isExecuted())
        delete mDeviceInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdDeviceInstanceAdd::redo() throw (Exception)
{
    mBoard.addDeviceInstance(*mDeviceInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mBoard.removeDeviceInstance(*mDeviceInstance);
        throw;
    }
}

void CmdDeviceInstanceAdd::undo() throw (Exception)
{
    mBoard.removeDeviceInstance(*mDeviceInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mBoard.addDeviceInstance(*mDeviceInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
