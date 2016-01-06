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

#ifndef LIBREPCB_PROJECT_CMDDEVICEINSTANCEADD_H
#define LIBREPCB_PROJECT_CMDDEVICEINSTANCEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Device;
}

namespace project {

class Board;
class ComponentInstance;
class DeviceInstance;

/*****************************************************************************************
 *  Class CmdDeviceInstanceAdd
 ****************************************************************************************/

/**
 * @brief The CmdDeviceInstanceAdd class
 */
class CmdDeviceInstanceAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdDeviceInstanceAdd(Board& board, ComponentInstance& comp,
                                      const Uuid& deviceUuid, const Uuid& footprintUuid,
                                      const Point& position = Point(),
                                      const Angle& rotation = Angle(), UndoCommand* parent = 0) throw (Exception);
        explicit CmdDeviceInstanceAdd(DeviceInstance& device, UndoCommand* parent = 0) throw (Exception);
        ~CmdDeviceInstanceAdd() noexcept;

        // Getters
        DeviceInstance* getDeviceInstance() const noexcept {return mDeviceInstance;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Board& mBoard;

        /// @brief The created device instance
        DeviceInstance* mDeviceInstance;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDDEVICEINSTANCEADD_H
