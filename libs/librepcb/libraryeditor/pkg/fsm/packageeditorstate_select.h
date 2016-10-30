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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_SELECT_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_SELECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "packageeditorstate.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

class CmdMoveSelectedFootprintItems;

/*****************************************************************************************
 *  Class PackageEditorState_Select
 ****************************************************************************************/

/**
 * @brief The PackageEditorState_Select class
 *
 * @author  ubruhin
 * @date    2017-05-28
 */
class PackageEditorState_Select final : public PackageEditorState
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        PackageEditorState_Select() = delete;
        PackageEditorState_Select(const PackageEditorState_Select& other) = delete;
        explicit PackageEditorState_Select(Context& context) noexcept;
        ~PackageEditorState_Select() noexcept;

        // Event Handlers
        bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneLeftMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneLeftMouseButtonDoubleClicked(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processRotateCw() noexcept override;
        bool processRotateCcw() noexcept override;
        bool processRemove() noexcept override;

        // Operator Overloadings
        PackageEditorState_Select& operator=(const PackageEditorState_Select& rhs) = delete;


    private: // Methods
        bool openContextMenuAtPos(const Point& pos) noexcept;
        bool openPropertiesDialogOfItemAtPos(const Point& pos) noexcept;
        bool rotateSelectedItems(const Angle& angle) noexcept;
        bool removeSelectedItems() noexcept;
        void setSelectionRect(const Point& p1, const Point& p2) noexcept;
        void clearSelectionRect(bool updateItemsSelectionState) noexcept;


    private: // Types / Data
        enum class SubState {IDLE, SELECTING, MOVING};
        SubState mState;
        QScopedPointer<CmdMoveSelectedFootprintItems> mCmdMoveSelectedItems;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_SELECT_H
