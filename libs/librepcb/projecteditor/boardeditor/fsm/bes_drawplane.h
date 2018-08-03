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

#ifndef LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H
#define LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/graphics/graphicslayername.h>
#include "bes_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsLayerComboBox;

namespace project {

class NetSignal;
class BI_Plane;
class CmdBoardPlaneEdit;

namespace editor {

/*****************************************************************************************
 *  Class BES_DrawPlane
 ****************************************************************************************/

/**
 * @brief The BES_DrawPlane class
 */
class BES_DrawPlane final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_DrawPlane(BoardEditor& editor, Ui::BoardEditor& editorUi,
                               GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_DrawPlane();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;


    private: // Methods
        ProcRetVal processSubStateIdle(BEE_Base* event) noexcept;
        ProcRetVal processSubStatePositioning(BEE_Base* event) noexcept;
        ProcRetVal processIdleSceneEvent(BEE_Base* event) noexcept;
        ProcRetVal processPositioningSceneEvent(BEE_Base* event) noexcept;
        bool start(Board& board, const Point& pos) noexcept;
        bool addSegment(Board& board, const Point& pos) noexcept;
        bool abort(bool showErrMsgBox) noexcept;
        void updateVertexPosition(const Point& cursorPos) noexcept;
        void layerComboBoxLayerChanged(const QString& layerName) noexcept;
        //void widthComboBoxTextChanged(const QString& width) noexcept;
        //void filledCheckBoxCheckedChanged(bool checked) noexcept;
        void makeSelectedLayerVisible() noexcept;
        void setNetSignal(NetSignal* netsignal) noexcept;


    private: // Types

        /// Internal FSM States (substates)
        enum class SubState {
            Idle,
            Positioning,
        };


    private: // Data

        // State
        SubState mSubState;
        NetSignal* mCurrentNetSignal;
        GraphicsLayerName mCurrentLayerName;
        BI_Plane* mCurrentPlane;
        CmdBoardPlaneEdit* mCmdEditCurrentPlane;
        Point mLastVertexPos;

        // Widgets for the command toolbar
        QList<QAction*> mActionSeparators;
        QLabel* mNetSignalLabel;
        QComboBox* mNetSignalComboBox;
        QLabel* mLayerLabel;
        GraphicsLayerComboBox* mLayerComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H
