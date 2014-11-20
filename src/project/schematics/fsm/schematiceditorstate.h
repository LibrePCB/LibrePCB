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

#ifndef PROJECT_SCHEMATICEDITORSTATE_H
#define PROJECT_SCHEMATICEDITORSTATE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "schematiceditorevent.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
class Circuit;
}

/*****************************************************************************************
 *  Class SchematicEditorState
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicEditorState class
 */
class SchematicEditorState : public QObject
{
        Q_OBJECT

    public:

        // FSM States
        enum State {
            State_Initial, // only for initialization and destruction
            State_Select,
            State_Move,
            State_DrawText,
            State_DrawRect,
            State_DrawPolygon,
            State_DrawCircle,
            State_DrawEllipse,
            State_DrawWire,
            State_AddComponent
        };

        // Constructors / Destructor
        explicit SchematicEditorState(SchematicEditor& editor, Ui::SchematicEditor& editorUi);
        virtual ~SchematicEditorState();

        // General Methods
        virtual State process(SchematicEditorEvent* event) noexcept = 0; // returns the next state
        virtual void entry(State previousState) noexcept;
        virtual void exit(State nextState) noexcept;

    protected:

        // General Attributes which are needed by some state objects
        Project& mProject;
        Circuit& mCircuit;
        SchematicEditor& mEditor;
        Ui::SchematicEditor& mEditorUi; ///< allows access to SchematicEditor UI
        State mCurrentState;
        QHash<State, SchematicEditorState*> mSubStates;
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITORSTATE_H
