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

#ifndef LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDIT_H
#define LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Device;

/*****************************************************************************************
 *  Class CmdDeviceInstanceEdit
 ****************************************************************************************/

/**
 * @brief The CmdDeviceInstanceEdit class
 */
class CmdDeviceInstanceEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdDeviceInstanceEdit(BI_Device& dev) noexcept;
        ~CmdDeviceInstanceEdit() noexcept;

        // General Methods
        void setPosition(Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept;
        void setRotation(const Angle& angle, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
        void setMirrored(bool mirrored, bool immediate) throw (Exception);
        void mirror(const Point& center, Qt::Orientation orientation, bool immediate) throw (Exception);


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        // Attributes from the constructor
        BI_Device& mDevice;

        // General Attributes
        Point mOldPos;
        Point mNewPos;
        Angle mOldRotation;
        Angle mNewRotation;
        bool mOldMirrored;
        bool mNewMirrored;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDIT_H
