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

#ifndef LIBREPCB_PROJECT_CMDREMOVEDEVICEFROMBOARD_H
#define LIBREPCB_PROJECT_CMDREMOVEDEVICEFROMBOARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Device;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveDeviceFromBoard
 ****************************************************************************************/

/**
 * @brief The CmdRemoveDeviceFromBoard class
 */
class CmdRemoveDeviceFromBoard final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdRemoveDeviceFromBoard() = delete;
        CmdRemoveDeviceFromBoard(const CmdRemoveDeviceFromBoard& other) = delete;
        CmdRemoveDeviceFromBoard(BI_Device& device) noexcept;
        ~CmdRemoveDeviceFromBoard() noexcept;

        // Operator Overloadings
        CmdRemoveDeviceFromBoard& operator=(const CmdRemoveDeviceFromBoard& other) = delete;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;


        // Private Member Variables

        // Attributes from the constructor
        BI_Device& mDevice;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVEDEVICEFROMBOARD_H
