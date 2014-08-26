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

#ifndef PROJECT_SCHEMATICEDITORFSM_H
#define PROJECT_SCHEMATICEDITORFSM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class SchematicEditor;
}

/*****************************************************************************************
 *  Class SchematicEditorFsm
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicEditorFsm class
 */
class SchematicEditorFsm final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicEditorFsm(SchematicEditor& editor) noexcept;
        ~SchematicEditorFsm() noexcept;

        // General Methods
        void processEvent(QEvent* event) noexcept;

    private:

        // Private Helper Methods
        void updateToolsToolbar();

        // General Attributes
        SchematicEditor& mEditor;

        // FSM States
        enum State {
            Initial,
            Select,
            Move,
            DrawWires,
            AddComponents
        };
        State mCurrentState;
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITORFSM_H
