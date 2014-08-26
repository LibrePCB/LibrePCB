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

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditorFsm::SchematicEditorFsm(SchematicEditor& editor) noexcept :
    QObject(0), mEditor(editor), mCurrentState(Initial)
{
    // go to state "Select"
    processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartSelect));
}

SchematicEditorFsm::~SchematicEditorFsm() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicEditorFsm::processEvent(QEvent* event) noexcept
{
    switch (static_cast<int>(event->type()))
    {
        case SchematicEditorEvent::AbortCommand:
        case SchematicEditorEvent::StartSelect:
            mCurrentState = Select;
            updateToolsToolbar();
            break;
        case SchematicEditorEvent::StartMove:
            mCurrentState = Move;
            updateToolsToolbar();
            break;
        case SchematicEditorEvent::StartDrawWires:
            mCurrentState = DrawWires;
            updateToolsToolbar();
            break;
        case SchematicEditorEvent::StartAddComponents:
            mCurrentState = AddComponents;
            updateToolsToolbar();
            break;
        default:
            break;
    }

    delete event;
}

/*****************************************************************************************
 *  Private Helper Methods
 ****************************************************************************************/

void SchematicEditorFsm::updateToolsToolbar()
{
    foreach (QAction* action, mEditor.mUi->toolsToolbar->actions())
    {
        action->setChecked(false);
        action->setCheckable(false);
    }

    QAction* active = 0;

    switch (mCurrentState)
    {
        case Select:
            active = mEditor.mUi->actionToolSelect;
            break;
        case Move:
            active = mEditor.mUi->actionToolMove;
            break;
        case DrawWires:
            active = mEditor.mUi->actionToolDrawWire;
            break;
        case AddComponents:
            active = mEditor.mUi->actionToolAddComponent;
            break;
        default:
            break;
    }

    if (active)
    {
        active->setCheckable(true);
        active->setChecked(true);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
