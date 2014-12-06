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
class SchematicNetLine;
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

        // Private Types

        /// Internal FSM States (substates)
        enum SubState {
            SubState_Idle,                  ///< idle state [initial state]
            SubState_PositioningNetPoint    ///< in this state, an undo command is active!
        };

        /**
         * @brief The WireMode enum contains all available wire modes
         *
         * @note The first item must have the value 0!
         */
        enum WireMode {
            WireMode_HV = 0,    ///< horizontal - vertical [default]
            WireMode_VH,        ///< vertical - horizontal
            WireMode_9045,      ///< 90° - 45°
            WireMode_4590,      ///< 45° - 90°
            WireMode_Straight,  ///< straight
            WireMode_COUNT      ///< count of wire modes
        };


        // Private Methods
        State processSubStateIdle(SchematicEditorEvent* event) noexcept;
        State processSubStatePositioning(SchematicEditorEvent* event) noexcept;
        State processIdleSceneEvent(SchematicEditorEvent* event) noexcept;
        State processPositioningSceneEvent(SchematicEditorEvent* event) noexcept;
        bool startPositioning(Schematic& schematic, const Point& pos,
                              SchematicNetPoint* fixedPoint = nullptr) noexcept;
        bool addNextNetPoint(Schematic& schematic, const Point& pos) noexcept;
        bool abortPositioning(bool showErrMsgBox) noexcept;
        void updateNetpointPositions(const Point& cursorPos) noexcept;
        void updateWireModeActionsCheckedState() noexcept;
        Point calcMiddlePointPos(const Point& p1, const Point p2, WireMode mode) const noexcept;


        // General Attributes
        SubState mSubState; ///< the current substate
        WireMode mWireMode; ///< the current wire mode
        SchematicNetPoint* mFixedNetPoint; ///< the fixed netpoint (start point of the line)
        SchematicNetLine* mPositioningNetLine1; ///< line between fixed point and p1
        SchematicNetPoint* mPositioningNetPoint1; ///< the first netpoint to place
        SchematicNetLine* mPositioningNetLine2; ///< line between p1 and p2
        SchematicNetPoint* mPositioningNetPoint2; ///< the second netpoint to place

        // Widgets for the command toolbar
        QHash<WireMode, QAction*> mWireModeActions;
        QList<QAction*> mActionSeparators;
        QLabel* mNetClassLabel;
        QComboBox* mNetClassComboBox;
        QMetaObject::Connection mNetClassAddCon;
        QMetaObject::Connection mNetClassRemoveCon;
        QLabel* mNetSignalLabel;
        QComboBox* mNetSignalComboBox;
        QMetaObject::Connection mNetSignalAddCon;
        QMetaObject::Connection mNetSignalRemoveCon;
        QLabel* mWidthLabel;
        QComboBox* mWidthComboBox;
};

} // namespace project

#endif // PROJECT_SES_DRAWWIRE_H
