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

#ifndef PROJECT_SES_FSM_H
#define PROJECT_SES_FSM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "ses_base.h"

/*****************************************************************************************
 *  Class SES_FSM
 ****************************************************************************************/

namespace project {

/**
 * @brief The SES_FSM (Schematic Editor Finite State Machine) class
 */
class SES_FSM final : public SES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_FSM(SchematicEditor& editor, Ui::SchematicEditor& editorUi) noexcept;
        ~SES_FSM() noexcept;

        // General Methods
        bool processEvent(SEE_Base* event, bool deleteEvent = false) noexcept;


    private:

        /// FSM States
        enum State {
            State_NoState,      ///< no state active
            State_Select,       ///< @see project#SES_Select
            State_Move,         ///< @see project#SES_Move
            State_DrawText,     ///< @see project#SES_DrawText
            State_DrawRect,     ///< @see project#SES_DrawRect
            State_DrawPolygon,  ///< @see project#SES_DrawPolygon
            State_DrawCircle,   ///< @see project#SES_DrawCircle
            State_DrawEllipse,  ///< @see project#SES_DrawEllipse
            State_DrawWire,     ///< @see project#SES_DrawWire
            State_AddNetLabel,  ///< @see project#SES_AddNetLabel
            State_AddComponent  ///< @see project#SES_AddComponents
        };


        // General Methods
        ProcRetVal process(SEE_Base* event) noexcept;
        State processEventFromChild(SEE_Base* event) noexcept; ///< returns the next state


        // Attributes
        State mCurrentState;
        State mPreviousState;
        QHash<State, SES_Base*> mSubStates;
};

} // namespace project

#endif // PROJECT_SES_FSM_H
