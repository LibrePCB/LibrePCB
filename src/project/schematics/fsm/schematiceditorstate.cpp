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
#include "schematiceditorstate.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditorState::SchematicEditorState(SchematicEditor& editor) :
    QObject(0), mEditor(editor), mCurrentState(State_Initial)
{
}

SchematicEditorState::~SchematicEditorState()
{
    // exit the current substate
    if (mSubStates.contains(mCurrentState))
        mSubStates[mCurrentState]->exit(State_Initial);

    mCurrentState = State_Initial; // switch to an invalid state

    // delete all substates
    qDeleteAll(mSubStates);     mSubStates.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicEditorState::entry(State previousState) noexcept
{
    Q_UNUSED(previousState);
}

void SchematicEditorState::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
