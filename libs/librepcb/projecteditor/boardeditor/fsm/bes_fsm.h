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

#ifndef LIBREPCB_PROJECT_BES_FSM_H
#define LIBREPCB_PROJECT_BES_FSM_H

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
namespace editor {

/*****************************************************************************************
 *  Class BES_FSM
 ****************************************************************************************/

/**
 * @brief The BES_FSM (Board Editor Finite State Machine) class
 */
class BES_FSM final : public BES_Base
{
        Q_OBJECT

    public:

        /// FSM States
        enum State {
            State_NoState,      ///< no state active
            State_Select,       ///< @see #project#BES_Select
            State_DrawTrace,    ///< @see #project#BES_DrawTrace
            State_AddVia,       ///< @see librepcb#project#BES_AddVia
            State_AddDevice,    ///< @see librepcb#project#BES_AddDevice
        };


        // Constructors / Destructor
        explicit BES_FSM(BoardEditor& editor, Ui::BoardEditor& editorUi,
                         GraphicsView& editorGraphicsView, UndoStack& undoStack) noexcept;
        ~BES_FSM() noexcept;

        // Getters
        State getCurrentState() const noexcept {return mCurrentState;}

        // General Methods
        bool processEvent(BEE_Base* event, bool deleteEvent = false) noexcept;


    signals:
        void stateChanged(State newState);


    private:

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept;
        State processEventFromChild(BEE_Base* event) noexcept; ///< returns the next state


        // Attributes
        State mCurrentState;
        State mPreviousState;
        QHash<State, BES_Base*> mSubStates;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BES_FSM_H
