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
#include "cmddeviceinstanceadd.h"
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

CmdDeviceInstanceAdd::CmdDeviceInstanceAdd(Board& board, ComponentInstance& comp,
        const Uuid& deviceUuid, const Uuid& footprintUuid, const Point& position,
        const Angle& rotation, bool mirror) noexcept :
    UndoCommand(tr("Add device to board")),
    mBoard(board), mComponentInstance(&comp), mDeviceUuid(deviceUuid),
    mFootprintUuid(footprintUuid), mPosition(position), mRotation(rotation),
    mMirror(mirror), mDeviceInstance(nullptr)
{
}

CmdDeviceInstanceAdd::CmdDeviceInstanceAdd(BI_Device& device) noexcept :
    UndoCommand(tr("Add device to board")),
    mBoard(device.getBoard()), mComponentInstance(nullptr), mDeviceUuid(),
    mFootprintUuid(), mPosition(), mRotation(), mMirror(),
    mDeviceInstance(&device)
{
}

CmdDeviceInstanceAdd::~CmdDeviceInstanceAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdDeviceInstanceAdd::performExecute() throw (Exception)
{
    mDeviceInstance = new BI_Device(mBoard, *mComponentInstance, mDeviceUuid,
                                    mFootprintUuid, mPosition, mRotation, mMirror); // can throw

    performRedo(); // can throw

    return true;
}

void CmdDeviceInstanceAdd::performUndo() throw (Exception)
{
    mBoard.removeDeviceInstance(*mDeviceInstance);
}

void CmdDeviceInstanceAdd::performRedo() throw (Exception)
{
    mBoard.addDeviceInstance(*mDeviceInstance);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
