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

#ifndef PROJECT_SES_DRAWWIRE_H
#define PROJECT_SES_DRAWWIRE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "schematiceditorstate.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class SchematicNetPoint;
}

/*****************************************************************************************
 *  Class SES_DrawWire
 ****************************************************************************************/

namespace project {


/**
 * @brief The SES_DrawWire class
 */
class SES_DrawWire final : public SchematicEditorState
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_DrawWire(SchematicEditor& editor, Ui::SchematicEditor& editorUi);
        ~SES_DrawWire();

        // General Methods
        State process(SchematicEditorEvent* event) noexcept;
        void entry(State previousState) noexcept;
        void exit(State nextState) noexcept;

    private:

        // Internal FSM States (substates)
        enum SubState {
            SubState_Idle, // initial substate
            SubState_PositioningNetPoint
        };
        SubState mSubState;
        SchematicNetPoint* mPositioningNetPoint;

        // Widgets for the command toolbar
        QLabel* mNetClassLabel;
        QComboBox* mNetClassComboBox;
        QLabel* mWidthLabel;
        QComboBox* mWidthComboBox;
};

} // namespace project

#endif // PROJECT_SES_DRAWWIRE_H
