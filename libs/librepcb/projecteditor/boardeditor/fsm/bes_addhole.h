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

#ifndef LIBREPCB_PROJECT_EDITOR_BES_ADDHOLE_H
#define LIBREPCB_PROJECT_EDITOR_BES_ADDHOLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_base.h"
#include <librepcb/project/boards/items/bi_hole.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class CmdHoleEdit;

namespace project {

class Board;

namespace editor {

/*****************************************************************************************
 *  Class BES_AddHole
 ****************************************************************************************/

/**
 * @brief The BES_AddHole class
 */
class BES_AddHole final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_AddHole(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_AddHole();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSceneEvent(BEE_Base* event) noexcept;
        bool addHole(Board& board, const Point& pos) noexcept;
        void updateHolePosition(const Point& pos) noexcept;
        bool fixHole(const Point& pos) noexcept;
        void diameterSpinBoxValueChanged(double value) noexcept;
        void makeLayerVisible() noexcept;


        // State
        bool mUndoCmdActive;
        BI_Hole* mHole;
        QScopedPointer<CmdHoleEdit> mEditCmd;
        PositiveLength mCurrentDiameter;

        // Widgets for the command toolbar
        QScopedPointer<QLabel> mDiameterLabel;
        QScopedPointer<QDoubleSpinBox> mDiameterSpinBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_EDITOR_BES_ADDHOLE_H
