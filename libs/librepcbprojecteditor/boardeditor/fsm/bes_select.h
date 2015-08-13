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

#ifndef PROJECT_BES_SELECT_H
#define PROJECT_BES_SELECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "bes_base.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class UndoCommand;

namespace project {
class CmdComponentInstanceEdit;
}

/*****************************************************************************************
 *  Class BES_Select
 ****************************************************************************************/

namespace project {

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
                                              Board* board) noexcept;
        ProcRetVal proccessIdleSceneRightClick(QGraphicsSceneMouseEvent* mouseEvent,
                                               Board* board) noexcept;
        ProcRetVal proccessIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                Board* board) noexcept;
        bool startMovingSelectedItems(Board* board) noexcept;
        bool rotateSelectedItems(const Angle& angle, Point center = Point(0, 0),
                                 bool centerOfElements = false) noexcept;
        bool removeSelectedItems() noexcept;


        // Types
        /// enum for all possible substates
        enum SubState {
            SubState_Idle,      ///< left mouse button is not pressed (default state)
            SubState_Moving     ///< left mouse button is pressed
        };


        // Attributes
        SubState mSubState;     ///< the current substate
        Point mLastMouseMoveDeltaPos;   ///< used in the moving substate (mapped to grid)
        UndoCommand* mParentCommand;    ///< the parent command for all moving commands
                                        ///< (nullptr if no command is active)
        QList<CmdComponentInstanceEdit*> mComponentEditCmds; ///< all footprint move commands
};

} // namespace project

#endif // PROJECT_BES_SELECT_H
