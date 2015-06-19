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

#ifndef PROJECT_BES_FSM_H
#define PROJECT_BES_FSM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "bes_base.h"

/*****************************************************************************************
 *  Class BES_FSM
 ****************************************************************************************/

namespace project {

/**
 * @brief The BES_FSM (Board Editor Finite State Machine) class
 */
class BES_FSM final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_FSM(BoardEditor& editor, Ui::BoardEditor& editorUi,
                         GraphicsView& editorGraphicsView) noexcept;
        ~BES_FSM() noexcept;

        // General Methods
        bool processEvent(BEE_Base* event, bool deleteEvent = false) noexcept;


    private:

        /// FSM States
        enum State {
            State_NoState,      ///< no state active
            State_Select,       ///< @see project#BES_Select
            //State_Move,         ///< @see project#BES_Move
            //State_DrawText,     ///< @see project#BES_DrawText
            //State_DrawRect,     ///< @see project#BES_DrawRect
            //State_DrawPolygon,  ///< @see project#BES_DrawPolygon
            //State_DrawCircle,   ///< @see project#BES_DrawCircle
            //State_DrawEllipse,  ///< @see project#BES_DrawEllipse
            //State_DrawWire,     ///< @see project#BES_DrawWire
            //State_AddNetLabel,  ///< @see project#BES_AddNetLabel
            //State_AddComponent  ///< @see project#BES_AddComponents
        };


        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept;
        State processEventFromChild(BEE_Base* event) noexcept; ///< returns the next state


        // Attributes
        State mCurrentState;
        State mPreviousState;
        QHash<State, BES_Base*> mSubStates;
};

} // namespace project

#endif // PROJECT_BES_FSM_H
