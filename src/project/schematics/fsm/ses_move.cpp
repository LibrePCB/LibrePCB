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
#include "ses_move.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Move::SES_Move(SchematicEditor& editor) :
    SchematicEditorState(editor)
{
}

SES_Move::~SES_Move()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SchematicEditorState::State SES_Move::process(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Move;

    switch (event->getType())
    {
        case SchematicEditorEvent::AbortCommand:
        case SchematicEditorEvent::StartSelect:
            nextState = State_Select;
            break;

        case SchematicEditorEvent::StartDrawText:
            nextState = State_DrawText;
            break;

        case SchematicEditorEvent::StartDrawRect:
            nextState = State_DrawRect;
            break;

        case SchematicEditorEvent::StartDrawPolygon:
            nextState = State_DrawPolygon;
            break;

        case SchematicEditorEvent::StartDrawCircle:
            nextState = State_DrawCircle;
            break;

        case SchematicEditorEvent::StartDrawEllipse:
            nextState = State_DrawEllipse;
            break;

        case SchematicEditorEvent::StartDrawWire:
            nextState = State_DrawWire;
            break;

        case SchematicEditorEvent::StartAddComponent:
            nextState = State_AddComponent;
            break;

        default:
            break;
    }

    return nextState;
}

void SES_Move::entry(State previousState) noexcept
{
    Q_UNUSED(previousState);

    mEditor.mUi->actionToolMove->setCheckable(true);
    mEditor.mUi->actionToolMove->setChecked(true);
}

void SES_Move::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    mEditor.mUi->actionToolMove->setCheckable(false);
    mEditor.mUi->actionToolMove->setChecked(false);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
