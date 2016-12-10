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

#ifndef LIBREPCB_PROJECT_SES_FSM_H
#define LIBREPCB_PROJECT_SES_FSM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "ses_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Class SES_FSM
 ****************************************************************************************/

/**
 * @brief The SES_FSM (Schematic Editor Finite State Machine) class
 */
class SES_FSM final : public SES_Base
{
        Q_OBJECT

    public:

        /// FSM States
        enum State {
            State_NoState,      ///< no state active
            State_Select,       ///< @see #project#SES_Select
            State_DrawWire,     ///< @see #project#SES_DrawWire
            State_AddNetLabel,  ///< @see #project#SES_AddNetLabel
            State_AddComponent  ///< @see #project#SES_AddComponent
        };

        // Constructors / Destructor
        explicit SES_FSM(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                         GraphicsView& editorGraphicsView, UndoStack& undoStack) noexcept;
        ~SES_FSM() noexcept;

        // Getters
        State getCurrentState() const noexcept {return mCurrentState;}

        // General Methods
        bool processEvent(SEE_Base* event, bool deleteEvent = false) noexcept;


    signals:
        void stateChanged(State newState);


    private:

        // General Methods
        ProcRetVal process(SEE_Base* event) noexcept;
        State processEventFromChild(SEE_Base* event) noexcept; ///< returns the next state


        // Attributes
        State mCurrentState;
        State mPreviousState;
        QHash<State, SES_Base*> mSubStates;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SES_FSM_H
