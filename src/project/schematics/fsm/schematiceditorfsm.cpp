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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QtEvents>
#include "schematiceditorfsm.h"
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
#include "ses_addcomponents.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditorFsm::SchematicEditorFsm(SchematicEditor& editor) noexcept :
    SchematicEditorState(editor)
{
    // create all substates
    mSubStates.insert(State_Select, new SES_Select(mEditor));
    mSubStates.insert(State_Move, new SES_Move(mEditor));
    mSubStates.insert(State_DrawText, new SES_DrawText(mEditor));
    mSubStates.insert(State_DrawRect, new SES_DrawRect(mEditor));
    mSubStates.insert(State_DrawPolygon, new SES_DrawPolygon(mEditor));
    mSubStates.insert(State_DrawCircle, new SES_DrawCircle(mEditor));
    mSubStates.insert(State_DrawEllipse, new SES_DrawEllipse(mEditor));
    mSubStates.insert(State_DrawWire, new SES_DrawWire(mEditor));
    mSubStates.insert(State_AddComponent, new SES_AddComponents(mEditor));

    // go to state "Select"
    mCurrentState = State_Select;
    mSubStates[mCurrentState]->entry(State_Initial);
}

SchematicEditorFsm::~SchematicEditorFsm() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool SchematicEditorFsm::processEvent(QEvent* event, bool deleteEvent) noexcept
{
    event->ignore();
    process(event); // the "isAccepted" flag is set here if the event was accepted
    bool accepted = event->isAccepted();
    if (deleteEvent)
        delete event;
    return accepted;
}

SchematicEditorState::State SchematicEditorFsm::process(QEvent* event) noexcept
{
    State next = mSubStates[mCurrentState]->process(event);

    if (next != mCurrentState)
    {
        Q_ASSERT(mSubStates.contains(next));

        // switch to the next state
        mSubStates[mCurrentState]->exit(next);
        mSubStates[next]->entry(mCurrentState);
        mCurrentState = next;
    }

    return mCurrentState; // this is not really used...
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
