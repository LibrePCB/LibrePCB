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

#ifndef LIBREPCB_PROJECT_BOARDEDITOREVENT_H
#define LIBREPCB_PROJECT_BOARDEDITOREVENT_H

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
class Board;
class ComponentInstance;

namespace editor {

class BoardEditor;

namespace Ui {
class BoardEditor;
}

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
            //StartMove,          ///< start command: move elements
            //StartDrawText,      ///< start command: draw text
            //StartDrawRect,      ///< start command: draw rect
            StartDrawPolygon,   ///< start command: draw polygon
            //StartDrawCircle,    ///< start command: draw circle
            //StartDrawEllipse,   ///< start command: draw ellipse
            StartDrawTrace,     ///< start command: draw trace
            StartAddVia,        ///< start command: add via
            //StartAddNetLabel,   ///< start command: add netlabel
            Edit_Copy,          ///< copy the selected elements to clipboard (ctrl+c)
            Edit_Cut,           ///< cut the selected elements (ctrl+x)
            Edit_Paste,         ///< paste the elements from the clipboard (ctrl+v)
            Edit_RotateCCW,     ///< rotate the selected elements 90° CCW (r)
            Edit_RotateCW,      ///< rotate the selected elements 90° CW (Shift+r)
            Edit_FlipHorizontal,///< flip the selected elements horizontal (f)
            Edit_FlipVertical,  ///< flip the selected elements vertical (Shift+f)
            Edit_Remove,        ///< remove the selected elements
            // Redirected QEvent's (SEE_RedirectedQEvent objects, with pointer to a QEvent)
            GraphicsViewEvent,  ///< event from #GraphicsView @see #project#SEE_RedirectedQEvent
            // Special Events (with some additional parameters)
            StartAddDevice,     ///< @see #project#BEE_StartAddDevice
            //SwitchToSchematicPage,  ///< @see #project#SEE_SwitchToSchematicPage
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
         * @param bee  A BEE_Base pointer to a BEE_RedirectedQEvent object
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

/*****************************************************************************************
 *  Class BEE_StartAddDevice
 ****************************************************************************************/

/**
 * @brief The BEE_StartAddDevice class
 *
 * @see librepcb#project#BES_AddDevice
 */
class BEE_StartAddDevice final : public BEE_Base
{
    public:

        // Constructors / Destructor
        BEE_StartAddDevice() = delete;
        BEE_StartAddDevice(const BEE_StartAddDevice& other) = delete;
        BEE_StartAddDevice(ComponentInstance& cmp, const Uuid& dev, const Uuid& fpt);
        ~BEE_StartAddDevice();

        // Getters
        ComponentInstance& getComponentInstance() const noexcept {return mComponentInstance;}
        const Uuid& getDeviceUuid() const noexcept {return mDeviceUuid;}
        const Uuid& getFootprintUuid() const noexcept {return mFootprintUuid;}

    private:

        ComponentInstance& mComponentInstance;
        Uuid mDeviceUuid;
        Uuid mFootprintUuid;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDEDITOREVENT_H
