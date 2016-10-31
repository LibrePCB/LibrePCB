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

#ifndef LIBREPCB_PROJECT_BES_ADDVIA_H
#define LIBREPCB_PROJECT_BES_ADDVIA_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_base.h"
#include <librepcb/project/boards/items/bi_via.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;
class BI_Via;
class CmdBoardViaEdit;

/*****************************************************************************************
 *  Class BES_AddVia
 ****************************************************************************************/

/**
 * @brief The BES_AddVia class
 */
class BES_AddVia final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_AddVia(BoardEditor& editor, Ui::BoardEditor& editorUi,
                            GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_AddVia();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSceneEvent(BEE_Base* event) noexcept;
        bool addVia(Board& board) noexcept;
        bool updateVia(Board& board, const Point& pos) noexcept;
        bool fixVia(const Point& pos) noexcept;
        void updateShapeActionsCheckedState() noexcept;


        // General Attributes
        bool mUndoCmdActive;
        BI_Via* mCurrentVia;
        BI_Via::Shape mCurrentViaShape;
        Length mCurrentViaSize;
        Length mCurrentViaDrillDiameter;
        NetSignal* mCurrentViaNetSignal;
        QScopedPointer<CmdBoardViaEdit> mEditCmd;

        // Widgets for the command toolbar
        QHash<int, QAction*> mShapeActions;
        QList<QAction*> mActionSeparators;
        QLabel* mSizeLabel;
        QComboBox* mSizeComboBox;
        QLabel* mDrillLabel;
        QComboBox* mDrillComboBox;
        QLabel* mNetSignalLabel;
        QComboBox* mNetSignalComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BES_ADDVIA_H
