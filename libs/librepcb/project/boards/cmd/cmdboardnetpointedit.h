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

#ifndef LIBREPCB_PROJECT_CMDBOARDNETPOINTEDIT_H
#define LIBREPCB_PROJECT_CMDBOARDNETPOINTEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>
#include <librepcb/common/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class BI_NetPoint;
class BI_FootprintPad;
class BI_Via;
class NetSignal;

/*****************************************************************************************
 *  Class CmdBoardNetPointEdit
 ****************************************************************************************/

/**
 * @brief The CmdBoardNetPointEdit class
 */
class CmdBoardNetPointEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdBoardNetPointEdit(BI_NetPoint& point) noexcept;
        ~CmdBoardNetPointEdit() noexcept;

        // Setters
        void setLayer(GraphicsLayer& layer) noexcept;
        void setNetSignal(NetSignal& netsignal) noexcept;
        void setPadToAttach(BI_FootprintPad* pad) noexcept;
        void setViaToAttach(BI_Via* via) noexcept;
        void setPosition(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        // Attributes from the constructor
        BI_NetPoint& mNetPoint;

        // General Attributes
        GraphicsLayer* mOldLayer;
        GraphicsLayer* mNewLayer;
        NetSignal* mOldNetSignal;
        NetSignal* mNewNetSignal;
        BI_FootprintPad* mOldFootprintPad;
        BI_FootprintPad* mNewFootprintPad;
        BI_Via* mOldVia;
        BI_Via* mNewVia;
        Point mOldPos;
        Point mNewPos;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDNETPOINTEDIT_H
