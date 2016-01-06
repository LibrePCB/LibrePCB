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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <QtEvents>
#include "bes_fsm.h"
#include "boardeditorevent.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include "bes_select.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_FSM::BES_FSM(BoardEditor& editor, Ui::BoardEditor& editorUi,
                 GraphicsView& editorGraphicsView, UndoStack& undoStack) noexcept :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mCurrentState(State_NoState), mPreviousState(State_NoState)
{
    // create all substates
    mSubStates.insert(State_Select, new BES_Select(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));

    // go to state "Select"
    if (mSubStates[State_Select]->entry(nullptr))
        mCurrentState = State_Select;
}

BES_FSM::~BES_FSM() noexcept
{
    // exit the current substate
    if (mCurrentState != State_NoState)
        mSubStates[mCurrentState]->exit(nullptr);
    // delete all substates
    qDeleteAll(mSubStates);     mSubStates.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool BES_FSM::processEvent(BEE_Base* event, bool deleteEvent) noexcept
{
    Q_ASSERT(event->isAccepted() == false);
    process(event); // the "isAccepted" flag is set here if the event was accepted
    bool accepted = event->isAccepted();
    if (deleteEvent) delete event;
    return accepted;
}

BES_Base::ProcRetVal BES_FSM::process(BEE_Base* event) noexcept
{
    State nextState = mCurrentState;
    ProcRetVal retval = PassToParentState;

    // let the current state process the event
    if (mCurrentState != State_NoState)
        retval = mSubStates[mCurrentState]->process(event);

    switch (retval)
    {
        case ForceStayInState:
            nextState = mCurrentState;
            event->setAccepted(true);
            break;
        case ForceLeaveState:
            nextState = (mPreviousState != State_NoState) ? mPreviousState : State_Select;
            event->setAccepted(true);
            break;
        case PassToParentState:
            nextState = processEventFromChild(event);
            break;
        default:
            Q_ASSERT(false);
            break;
    }

    // switch to the next state, if needed
    if (nextState != mCurrentState)
    {
        if (mCurrentState != State_NoState)
        {
            // leave the current state
            if (mSubStates[mCurrentState]->exit(event))
            {
                mPreviousState = mCurrentState;
                mCurrentState = State_NoState;
            }
        }
        if ((mCurrentState == State_NoState) && (nextState != State_NoState))
        {
            // entry the next state
            if (mSubStates[nextState]->entry(event))
                mCurrentState = nextState;
            else // use the select state as fallback
                processEvent(new BEE_Base(BEE_Base::StartSelect), true);
        }
    }

    return ForceStayInState; // this is not used...
}

BES_FSM::State BES_FSM::processEventFromChild(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::AbortCommand:
        case BEE_Base::StartSelect:
            event->setAccepted(true);
            return State_Select;
        /*case SEE_Base::StartMove:
            event->setAccepted(true);
            return State_Move;
        case SEE_Base::StartDrawText:
            event->setAccepted(true);
            return State_DrawText;
        case SEE_Base::StartDrawRect:
            event->setAccepted(true);
            return State_DrawRect;
        case SEE_Base::StartDrawPolygon:
            event->setAccepted(true);
            return State_DrawPolygon;
        case SEE_Base::StartDrawCircle:
            event->setAccepted(true);
            return State_DrawCircle;
        case SEE_Base::StartDrawEllipse:
            event->setAccepted(true);
            return State_DrawEllipse;
        case SEE_Base::StartDrawWire:
            event->setAccepted(true);
            return State_DrawWire;
        case SEE_Base::StartAddNetLabel:
            event->setAccepted(true);
            return State_AddNetLabel;
        case SEE_Base::StartAddComponent:
            event->setAccepted(true);
            return State_AddComponent;
        case SEE_Base::SwitchToSchematicPage:
            event->setAccepted(true);
            return mCurrentState;*/
        case BEE_Base::GraphicsViewEvent:
        {
            QEvent* e = BEE_RedirectedQEvent::getQEventFromBEE(event);
            Q_ASSERT(e); if (!e) return mCurrentState;
            if ((e->type() == QEvent::GraphicsSceneMousePress) ||
                (e->type() == QEvent::GraphicsSceneMouseDoubleClick))
            {
                QGraphicsSceneMouseEvent* e2 = dynamic_cast<QGraphicsSceneMouseEvent*>(e);
                Q_ASSERT(e2); if (!e2) return mCurrentState;
                if (e2->buttons() == Qt::RightButton)
                    return (mPreviousState != State_NoState) ? mPreviousState : State_Select;
                else
                    return mCurrentState;
            }
        }
        default:
            return mCurrentState;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
