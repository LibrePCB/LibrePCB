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

#ifndef PROJECT_CMDDEVICEINSTANCEREMOVE_H
#define PROJECT_CMDDEVICEINSTANCEREMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Board;
class DeviceInstance;
}

/*****************************************************************************************
 *  Class CmdDeviceInstanceRemove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdDeviceInstanceRemove class
 */
class CmdDeviceInstanceRemove final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdDeviceInstanceRemove(Board& board, DeviceInstance& dev,
                                         UndoCommand* parent = 0) throw (Exception);
        ~CmdDeviceInstanceRemove() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;


    private:

        // Attributes from the constructor
        Board& mBoard;
        DeviceInstance& mDevice;
};

} // namespace project

#endif // PROJECT_CMDDEVICEINSTANCEREMOVE_H
