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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDFOOTPRINTITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDFOOTPRINTITEMS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>
#include "../packageeditorstate.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class CmdEllipseEdit;
class CmdTextEdit;
class CmdPolygonMove;
class CmdHoleEdit;

namespace library {

class CmdFootprintPadEdit;

namespace editor {

/*****************************************************************************************
 *  Class CmdMoveSelectedFootprintItems
 ****************************************************************************************/

/**
 * @brief The CmdMoveSelectedFootprintItems class
 *
 * @author  ubruhin
 * @date    2017-05-28
 */
class CmdMoveSelectedFootprintItems final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdMoveSelectedFootprintItems() = delete;
        CmdMoveSelectedFootprintItems(const CmdMoveSelectedFootprintItems& other) = delete;
        CmdMoveSelectedFootprintItems(const PackageEditorState::Context& context,
                                      const Point& startPos) noexcept;
        ~CmdMoveSelectedFootprintItems() noexcept;

        // General Methods
        void setCurrentPosition(const Point& pos) noexcept;

        // Operator Overloadings
        CmdMoveSelectedFootprintItems& operator=(const CmdMoveSelectedFootprintItems& rhs) = delete;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        void deleteAllCommands() noexcept;


        // Private Member Variables
        const PackageEditorState::Context& mContext;
        Point mStartPos;
        Point mDeltaPos;

        // Move commands
        QList<CmdFootprintPadEdit*> mPadEditCmds;
        QList<CmdEllipseEdit*> mEllipseEditCmds;
        QList<CmdPolygonMove*> mPolygonEditCmds;
        QList<CmdTextEdit*> mTextEditCmds;
        QList<CmdHoleEdit*> mHoleEditCmds;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDFOOTPRINTITEMS_H
