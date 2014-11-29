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
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
class Schematic;
class SchematicEditor;
}

namespace Ui {
class SchematicEditor;
}

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
            // Triggered Actions (SchematicEditorEvent objects, no additional parameters)
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
            // Redirected QEvent's (SEE_RedirectedQEvent objects, with pointer to a QEvent)
            SchematicSceneEvent,
            // Special Events (with some additional parameters)
            SetAddComponentParams, ///< @see project#SEE_SetAddComponentParams
            SwitchToSchematicPage, ///< @see project#SEE_SwitchToSchematicPage
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

        // Static Methods

        /**
         * @brief Helper method to get the QEvent from a pointer to SchematicEditorEvent
         * @param see   A SchematicEditorEvent pointer to a SEE_RedirectedQEvent object
         * @return @li the pointer to the QEvent (if "see" was a pointer to a
         *             SEE_RedirectedQEvent object)
         *         @li nullptr otherwise
         */
        static QEvent* getQEventFromSEE(const SchematicEditorEvent* see) noexcept
        {
            const SEE_RedirectedQEvent* r = dynamic_cast<const SEE_RedirectedQEvent*>(see);
            return (r ? r->getQEvent() : nullptr);
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
 * An event of this type must be created and passed to the FSM after entering the state
 * project#SES_AddComponents to specify which component should be added to the
 * circuit. The symbols of that component will then be added to the active schematic.
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

/*****************************************************************************************
 *  Class SEE_SwitchToSchematicPage
 ****************************************************************************************/

/**
 * @brief The SEE_SwitchToSchematicPage class
 *
 * If someone (the user or the application) wants to switch to another schematic page in
 * the schematic editor, this is not allowed at any time (for example, while drawing a
 * netline in the active schematic, you cannot switch to another schematic). So this type
 * of event must be processed by the FSM. The FSM then will only decide whether changing
 * the schematic is allowed (event accepted) or not (event rejected). If the event was
 * accepted, the schematic editor then will switch to the requested schematic page.
 *
 * @see project#SchematicEditor#setActiveSchematicIndex()
 */
class SEE_SwitchToSchematicPage : public SchematicEditorEvent
{
    public:

        // Constructors / Destructor
        SEE_SwitchToSchematicPage(unsigned int schematicIndex) :
            SchematicEditorEvent(SwitchToSchematicPage),
            mSchematicIndex(schematicIndex) {}
        virtual ~SEE_SwitchToSchematicPage() {}

        // Getters
        unsigned int getSchematicIndex() const noexcept {return mSchematicIndex;}

    private:

        unsigned int mSchematicIndex; ///< the requested schematic page index
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITOREVENT_H
