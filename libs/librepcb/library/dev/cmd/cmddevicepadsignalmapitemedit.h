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

#ifndef LIBREPCB_LIBRARY_CMDDEVICEPADSIGNALMAPITEMEDIT_H
#define LIBREPCB_LIBRARY_CMDDEVICEPADSIGNALMAPITEMEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

class DevicePadSignalMapItem;

/*****************************************************************************************
 *  Class CmdDevicePadSignalMapItemEdit
 ****************************************************************************************/

/**
 * @brief The CmdDevicePadSignalMapItemEdit class
 */
class CmdDevicePadSignalMapItemEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdDevicePadSignalMapItemEdit() = delete;
        CmdDevicePadSignalMapItemEdit(const CmdDevicePadSignalMapItemEdit& other) = delete;
        explicit CmdDevicePadSignalMapItemEdit(DevicePadSignalMapItem& item) noexcept;
        ~CmdDevicePadSignalMapItemEdit() noexcept;

        // Setters
        void setSignalUuid(const Uuid& uuid) noexcept;

        // Operator Overloadings
        CmdDevicePadSignalMapItemEdit& operator=(const CmdDevicePadSignalMapItemEdit& rhs) = delete;


    private: // Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


    private: // Data
        DevicePadSignalMapItem& mItem;

        Uuid mOldSignalUuid;
        Uuid mNewSignalUuid;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_CMDDEVICEPADSIGNALMAPITEMEDIT_H
