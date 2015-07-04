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
#include "ses_fsm.h"
#include "schematiceditorevent.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "ses_select.h"
#include "ses_move.h"
#include "ses_drawtext.h"
#include "ses_drawrect.h"
#include "ses_drawpolygon.h"
#include "ses_drawcircle.h"
#include "ses_drawellipse.h"
#include "ses_drawwire.h"
#include "ses_addnetlabel.h"
#include "ses_addcomponents.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_FSM::SES_FSM(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                 GraphicsView& editorGraphicsView, UndoStack& undoStack) noexcept :
    SES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mCurrentState(State_NoState), mPreviousState(State_NoState)
{
    // create all substates
    mSubStates.insert(State_Select, new SES_Select(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_Move, new SES_Move(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawText, new SES_DrawText(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawRect, new SES_DrawRect(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawPolygon, new SES_DrawPolygon(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawCircle, new SES_DrawCircle(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawEllipse, new SES_DrawEllipse(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_DrawWire, new SES_DrawWire(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_AddNetLabel, new SES_AddNetLabel(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));
    mSubStates.insert(State_AddComponent, new SES_AddComponents(mEditor, mEditorUi, mEditorGraphicsView, mUndoStack));

    // go to state "Select"
    if (mSubStates[State_Select]->entry(nullptr))
        mCurrentState = State_Select;
}

SES_FSM::~SES_FSM() noexcept
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

bool SES_FSM::processEvent(SEE_Base* event, bool deleteEvent) noexcept
{
    Q_ASSERT(event->isAccepted() == false);
    process(event); // the "isAccepted" flag is set here if the event was accepted
    bool accepted = event->isAccepted();
    if (deleteEvent) delete event;
    return accepted;
}

SES_Base::ProcRetVal SES_FSM::process(SEE_Base* event) noexcept
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
                processEvent(new SEE_Base(SEE_Base::StartSelect), true);
        }
    }

    return ForceStayInState; // this is not used...
}

SES_FSM::State SES_FSM::processEventFromChild(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::AbortCommand:
        case SEE_Base::StartSelect:
            event->setAccepted(true);
            return State_Select;
        case SEE_Base::StartMove:
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
            return mCurrentState;
        case SEE_Base::GraphicsViewEvent:
        {
            QEvent* e = SEE_RedirectedQEvent::getQEventFromSEE(event);
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
