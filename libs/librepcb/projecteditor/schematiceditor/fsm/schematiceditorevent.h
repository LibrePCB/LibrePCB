/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_SCHEMATICEDITOREVENT_H
#define LIBREPCB_PROJECT_SCHEMATICEDITOREVENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Project;
class Schematic;

namespace editor {

class SchematicEditor;

namespace Ui {
class SchematicEditor;
}

/*****************************************************************************************
 *  Class SEE_Base
 ****************************************************************************************/

/**
 * @brief The SEE_Base (Schematic Editor Event Base) class
 */
class SEE_Base
{
    public:

        /// FSM event types
        enum EventType_t {
            // Triggered Actions (SEE_Base objects, no additional parameters)
            AbortCommand,       ///< abort the currently active command (esc)
            StartSelect,        ///< start command: select elements
            StartDrawWire,      ///< start command: draw wire
            StartAddNetLabel,   ///< start command: add netlabel
            Edit_Copy,          ///< copy the selected elements to clipboard (ctrl+c)
            Edit_Cut,           ///< cut the selected elements (ctrl+x)
            Edit_Paste,         ///< paste the elements from the clipboard (ctrl+v)
            Edit_RotateCCW,     ///< rotate the selected elements 90° CCW (r)
            Edit_RotateCW,      ///< rotate the selected elements 90° CW (Shift+r)
            Edit_Mirror,        ///< mirror selected items (horizontally)
            Edit_Remove,        ///< remove the selected elements
            // Redirected QEvent's (SEE_RedirectedQEvent objects, with pointer to a QEvent)
            GraphicsViewEvent,  ///< event from #GraphicsView @see #project#SEE_RedirectedQEvent
            // Special Events (with some additional parameters)
            StartAddComponent,      ///< @see #project#SEE_StartAddComponent
            SwitchToSchematicPage,  ///< @see #project#SEE_SwitchToSchematicPage
        };

        // Constructors / Destructor
        SEE_Base(EventType_t type);
        virtual ~SEE_Base();

        // Getters
        EventType_t getType() const noexcept {return mType;}
        bool isAccepted() const noexcept {return mAccepted;}

        // Setters
        virtual void setAccepted(bool accepted) noexcept {mAccepted = accepted;}

    protected:

        EventType_t mType;
        bool mAccepted;
};

/*****************************************************************************************
 *  Class SEE_RedirectedQEvent
 ****************************************************************************************/

/**
 * @brief The SEE_RedirectedQEvent class
 */
class SEE_RedirectedQEvent final : public SEE_Base
{
    public:

        // Constructors / Destructor
        SEE_RedirectedQEvent(EventType_t type, QEvent* event) :
            SEE_Base(type), mQEvent(event) {}
        virtual ~SEE_RedirectedQEvent() {}

        // Getters
        QEvent* getQEvent() const noexcept {return mQEvent;}

        // Setters
        void setAccepted(bool accepted) noexcept
        {
            mQEvent->setAccepted(accepted);
            SEE_Base::setAccepted(accepted);
        }

        // Static Methods

        /**
         * @brief Helper method to get the QEvent from a pointer to SEE_Base
         * @param see  A SEE_Base pointer to a SEE_RedirectedQEvent object
         * @return @li the pointer to the QEvent (if "see" was a pointer to a
         *             SEE_RedirectedQEvent object)
         *         @li nullptr otherwise
         */
        static QEvent* getQEventFromSEE(const SEE_Base* see) noexcept
        {
            const SEE_RedirectedQEvent* r = dynamic_cast<const SEE_RedirectedQEvent*>(see);
            return (r ? r->getQEvent() : nullptr);
        }

    private:

        QEvent* mQEvent;
};

/*****************************************************************************************
 *  Class SEE_StartAddComponent
 ****************************************************************************************/

/**
 * @brief The SEE_StartAddComponent class
 *
 * @see #project#SES_AddComponent
 */
class SEE_StartAddComponent final : public SEE_Base
{
    public:

        // Constructors / Destructor
        SEE_StartAddComponent();
        SEE_StartAddComponent(const Uuid& cmp, const Uuid& symbVar);
        ~SEE_StartAddComponent();

        // Getters
        const tl::optional<Uuid>& getComponentUuid() const noexcept {return mComponentUuid;}
        const tl::optional<Uuid>& getSymbVarUuid() const noexcept {return mSymbVarUuid;}

    private:

        tl::optional<Uuid> mComponentUuid;
        tl::optional<Uuid> mSymbVarUuid;
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
 * @see #project#SchematicEditor#setActiveSchematicIndex()
 */
class SEE_SwitchToSchematicPage final : public SEE_Base
{
    public:

        // Constructors / Destructor
        SEE_SwitchToSchematicPage(unsigned int schematicIndex) :
            SEE_Base(EventType_t::SwitchToSchematicPage),
            mSchematicIndex(schematicIndex) {}
        virtual ~SEE_SwitchToSchematicPage() {}

        // Getters
        unsigned int getSchematicIndex() const noexcept {return mSchematicIndex;}

    private:

        unsigned int mSchematicIndex; ///< the requested schematic page index
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SCHEMATICEDITOREVENT_H
