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

#ifndef PROJECT_BOARDEDITOREVENT_H
#define PROJECT_BOARDEDITOREVENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
class Board;
class BoardEditor;
}

namespace Ui {
class BoardEditor;
}

namespace project {

/*****************************************************************************************
 *  Class BEE_Base
 ****************************************************************************************/

/**
 * @brief The BEE_Base (Board Editor Event Base) class
 */
class BEE_Base
{
    public:

        /// FSM event types
        enum EventType_t {
            // Triggered Actions (SEE_Base objects, no additional parameters)
            AbortCommand,       ///< abort the currently active command (esc)
            StartSelect,        ///< start command: select elements
            StartMove,          ///< start command: move elements
            StartDrawText,      ///< start command: draw text
            StartDrawRect,      ///< start command: draw rect
            StartDrawPolygon,   ///< start command: draw polygon
            StartDrawCircle,    ///< start command: draw circle
            StartDrawEllipse,   ///< start command: draw ellipse
            StartDrawWire,      ///< start command: draw wire
            StartAddNetLabel,   ///< start command: add netlabel
            Edit_Copy,          ///< copy the selected elements to clipboard (ctrl+c)
            Edit_Cut,           ///< cut the selected elements (ctrl+x)
            Edit_Paste,         ///< paste the elements from the clipboard (ctrl+v)
            Edit_RotateCW,      ///< rotate the selected elements 90° CW
            Edit_RotateCCW,     ///< rotate the selected elements 90° CCW
            Edit_Remove,        ///< remove the selected elements
            // Redirected QEvent's (SEE_RedirectedQEvent objects, with pointer to a QEvent)
            GraphicsViewEvent,  ///< event from #GraphicsView @see project#SEE_RedirectedQEvent
            // Special Events (with some additional parameters)
            StartAddComponent,      ///< @see project#SEE_StartAddComponent
            SwitchToSchematicPage,  ///< @see project#SEE_SwitchToSchematicPage
        };

        // Constructors / Destructor
        BEE_Base(EventType_t type);
        virtual ~BEE_Base();

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
 *  Class BEE_RedirectedQEvent
 ****************************************************************************************/

/**
 * @brief The BEE_RedirectedQEvent class
 */
class BEE_RedirectedQEvent final : public BEE_Base
{
    public:

        // Constructors / Destructor
        BEE_RedirectedQEvent(EventType_t type, QEvent* event) :
            BEE_Base(type), mQEvent(event) {}
        virtual ~BEE_RedirectedQEvent() {}

        // Getters
        QEvent* getQEvent() const noexcept {return mQEvent;}

        // Setters
        void setAccepted(bool accepted) noexcept
        {
            mQEvent->setAccepted(accepted);
            BEE_Base::setAccepted(accepted);
        }

        // Static Methods

        /**
         * @brief Helper method to get the QEvent from a pointer to BEE_Base
         * @param see  A BEE_Base pointer to a BEE_RedirectedQEvent object
         * @return @li the pointer to the QEvent (if "see" was a pointer to a
         *             BEE_RedirectedQEvent object)
         *         @li nullptr otherwise
         */
        static QEvent* getQEventFromBEE(const BEE_Base* bee) noexcept
        {
            const BEE_RedirectedQEvent* r = dynamic_cast<const BEE_RedirectedQEvent*>(bee);
            return (r ? r->getQEvent() : nullptr);
        }

    private:

        QEvent* mQEvent;
};

} // namespace project

#endif // PROJECT_BOARDEDITOREVENT_H
