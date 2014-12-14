/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_SES_SELECT_H
#define PROJECT_SES_SELECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "ses_base.h"
#include "../../../common/units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class UndoCommand;

namespace project {
class SymbolInstance;
class SchematicNetPoint;
class CmdSymbolInstanceMove;
class CmdSchematicNetPointMove;
}

/*****************************************************************************************
 *  Class SES_Select
 ****************************************************************************************/

namespace project {


/**
 * @brief The SES_Select class (default state of the schematic editor FSM)
 */
class SES_Select final : public SES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_Select(SchematicEditor& editor, Ui::SchematicEditor& editorUi);
        ~SES_Select();

        // General Methods
        ProcRetVal process(SEE_Base* event) noexcept override;
        bool entry(SEE_Base* event) noexcept override;
        bool exit(SEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSubStateIdle(SEE_Base* event) noexcept;
        ProcRetVal processSubStateIdleSceneEvent(SEE_Base* event) noexcept;
        ProcRetVal processSubStateMoving(SEE_Base* event) noexcept;
        ProcRetVal processSubStateMovingSceneEvent(SEE_Base* event) noexcept;
        ProcRetVal proccessIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                              Schematic* schematic) noexcept;
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
        Point mMoveStartPos;    ///< the scene position where the left mouse button was
                                ///< pressed (not mapped to grid!)
        Point mLastMouseMoveDeltaPos;   ///< used in the moving substate (mapped to grid)
        UndoCommand* mParentCommand;    ///< the parent command for all moving commands
                                        ///< (nullptr if no command is active)
        QList<CmdSymbolInstanceMove*> mSymbolMoveCmds; ///< all symbol move commands
        QList<CmdSchematicNetPointMove*> mNetPointMoveCmds; ///< all netpoint move commands
};

} // namespace project

#endif // PROJECT_SES_SELECT_H
