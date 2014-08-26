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

#ifndef PROJECT_SCHEMATICEDITOREVENT_H
#define PROJECT_SCHEMATICEDITOREVENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Class SchematicEditorEvent
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicEditorEvent class
 */
class SchematicEditorEvent : public QEvent
{
    public:

        // FSM events (codes used for QEvent::type)
        enum EventType {
            _First = QEvent::User, // the first user defined type after all Qt types
            AbortCommand,
            StartSelect,
            StartMove,
            StartDrawWires,
            StartAddComponents
        };

        // Constructors / Destructor
        SchematicEditorEvent(EventType type);
        virtual ~SchematicEditorEvent();
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITOREVENT_H
