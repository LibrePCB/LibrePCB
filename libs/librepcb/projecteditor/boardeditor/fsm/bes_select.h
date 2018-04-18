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

#ifndef LIBREPCB_PROJECT_BES_SELECT_H
#define LIBREPCB_PROJECT_BES_SELECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class UndoCommandGroup;
class Polygon;
class StrokeText;

namespace project {

class BI_Device;
class BI_Via;
class BI_Plane;

namespace editor {

class CmdMoveSelectedBoardItems;

/*****************************************************************************************
 *  Class BES_Select
 ****************************************************************************************/

/**
 * @brief The BES_Select class
 */
class BES_Select final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                            GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_Select();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSubStateIdle(BEE_Base* event) noexcept;
        ProcRetVal processSubStateIdleSceneEvent(BEE_Base* event) noexcept;
        ProcRetVal processSubStateMoving(BEE_Base* event) noexcept;
        ProcRetVal processSubStateMovingSceneEvent(BEE_Base* event) noexcept;
        ProcRetVal proccessIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                              Board& board) noexcept;
        ProcRetVal proccessIdleSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent* mouseEvent,
                                                             Board* board) noexcept;
        ProcRetVal proccessIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                Board* board) noexcept;
        bool startMovingSelectedItems(Board& board, const Point& startPos) noexcept;
        bool rotateSelectedItems(const Angle& angle) noexcept;
        bool flipSelectedItems(Qt::Orientation orientation) noexcept;
        bool removeSelectedItems() noexcept;
        void openDevicePropertiesDialog(BI_Device& device) noexcept;
        void openViaPropertiesDialog(BI_Via& via) noexcept;
        void openPlanePropertiesDialog(BI_Plane& plane) noexcept;
        void openPolygonPropertiesDialog(Board& board, Polygon& polygon) noexcept;
        void openStrokeTextPropertiesDialog(Board& board, StrokeText& text) noexcept;


        // Types
        /// enum for all possible substates
        enum SubState {
            SubState_Idle,      ///< left mouse button is not pressed (default state)
            SubState_Moving     ///< left mouse button is pressed
        };


        // Attributes
        SubState mSubState;     ///< the current substate
        QScopedPointer<CmdMoveSelectedBoardItems> mSelectedItemsMoveCommand;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BES_SELECT_H
