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
#include "ses_select.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Select::SES_Select(SchematicEditor& editor) :
    SchematicEditorState(editor), mPreviousState(State_Initial)
{
}

SES_Select::~SES_Select()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SchematicEditorState::State SES_Select::process(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Select;

    switch (event->getType())
    {
        case SchematicEditorEvent::StartMove:
            nextState = State_Move;
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

        case SchematicEditorEvent::SchematicSceneEvent:
        {
            QEvent* qevent = dynamic_cast<SEE_RedirectedQEvent*>(event)->getQEvent();
            switch (qevent->type())
            {
                case QEvent::GraphicsSceneMousePress:
                {
                    QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);

                    switch (sceneEvent->button())
                    {
                        case Qt::RightButton:
                        {
                            // switch back to the last command (previous state)
                            nextState = mPreviousState;
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return nextState;
}

void SES_Select::entry(State previousState) noexcept
{
    mPreviousState = previousState;

    mEditor.mUi->actionToolSelect->setCheckable(true);
    mEditor.mUi->actionToolSelect->setChecked(true);
}

void SES_Select::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    mEditor.mUi->actionToolSelect->setCheckable(false);
    mEditor.mUi->actionToolSelect->setChecked(false);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
