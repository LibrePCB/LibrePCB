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

namespace project {

/*****************************************************************************************
 *  Class SchematicEditorEvent
 ****************************************************************************************/

/**
 * @brief The SchematicEditorEvent class
 */
class SchematicEditorEvent
{
    public:

        // FSM event types
        enum EventType {
            // Triggered Actions (SchematicEditorEvent objects)
            AbortCommand,
            StartSelect,
            StartMove,
            StartDrawText,
            StartDrawRect,
            StartDrawPolygon,
            StartDrawCircle,
            StartDrawEllipse,
            StartDrawWire,
            StartAddComponent,
            // Redirected QEvent's (SEE_RedirectedQEvent objects)
            SchematicSceneEvent,
            // Special Events
            SetAddComponentParams, ///< @see project#SEE_SetAddComponentParams
        };

        // Constructors / Destructor
        SchematicEditorEvent(EventType type);
        virtual ~SchematicEditorEvent();

        // Getters
        EventType getType() const noexcept {return mType;}
        bool isAccepted() const noexcept {return mAccepted;}

        // Setters
        virtual void setAccepted(bool accepted) noexcept {mAccepted = accepted;}

    protected:

        EventType mType;
        bool mAccepted;
};

/*****************************************************************************************
 *  Class SEE_RedirectedQEvent
 ****************************************************************************************/

/**
 * @brief The SEE_RedirectedQEvent class
 */
class SEE_RedirectedQEvent : public SchematicEditorEvent
{
    public:

        // Constructors / Destructor
        SEE_RedirectedQEvent(EventType type, QEvent* event) :
            SchematicEditorEvent(type), mQEvent(event) {}
        virtual ~SEE_RedirectedQEvent() {}

        // Getters
        QEvent* getQEvent() const noexcept {return mQEvent;}

        // Setters
        void setAccepted(bool accepted) noexcept
        {
            mQEvent->setAccepted(accepted);
            SchematicEditorEvent::setAccepted(accepted);
        }

    private:

        QEvent* mQEvent;
};

/*****************************************************************************************
 *  Class SEE_SetAddComponentParams
 ****************************************************************************************/

/**
 * @brief The SEE_SetAddComponentParams class
 *
 * @see project#SES_AddComponents
 */
class SEE_SetAddComponentParams : public SchematicEditorEvent
{
    public:

        // Constructors / Destructor
        SEE_SetAddComponentParams(const QUuid& genComp, const QUuid& symbVar) :
            SchematicEditorEvent(SetAddComponentParams),
            mGenCompUuid(genComp), mSymbVarUuid(symbVar) {}
        virtual ~SEE_SetAddComponentParams() {}

        // Getters
        const QUuid& getGenCompUuid() const noexcept {return mGenCompUuid;}
        const QUuid& getSymbVarUuid() const noexcept {return mSymbVarUuid;}

    private:

        QUuid mGenCompUuid;
        QUuid mSymbVarUuid;
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITOREVENT_H
