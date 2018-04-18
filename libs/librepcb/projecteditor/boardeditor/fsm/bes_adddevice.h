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

#ifndef LIBREPCB_PROJECT_BES_ADDDEVICE_H
#define LIBREPCB_PROJECT_BES_ADDDEVICE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;
class ComponentInstance;
class BI_Device;
class CmdDeviceInstanceEditAll;

namespace editor {

/*****************************************************************************************
 *  Class BES_AddDevice
 ****************************************************************************************/

/**
 * @brief The BES_AddDevice class
 */
class BES_AddDevice final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BES_AddDevice(BoardEditor& editor, Ui::BoardEditor& editorUi,
                      GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_AddDevice();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSceneEvent(BEE_Base* event) noexcept;
        void startAddingDevice(ComponentInstance& cmp, const Uuid& dev, const Uuid& fpt);
        bool abortCommand(bool showErrMsgBox) noexcept;
        void rotateDevice(const Angle& angle) noexcept;
        void mirrorDevice(Qt::Orientation orientation) noexcept;


        // General Attributes
        bool mIsUndoCmdActive;

        // information about the current device to place
        BI_Device* mCurrentDeviceToPlace;
        QScopedPointer<CmdDeviceInstanceEditAll> mCurrentDeviceEditCmd;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BES_ADDDEVICE_H
