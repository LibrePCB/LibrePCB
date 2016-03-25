/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBREPCB_PROJECT_CMDBOARDNETPOINTADD_H
#define LIBREPCB_PROJECT_CMDBOARDNETPOINTADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class BoardLayer;

namespace project {

class NetSignal;
class Board;
class BI_FootprintPad;
class BI_NetPoint;
class BI_Via;

/*****************************************************************************************
 *  Class CmdBoardNetPointAdd
 ****************************************************************************************/

/**
 * @brief The CmdBoardNetPointAdd class
 */
class CmdBoardNetPointAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdBoardNetPointAdd(BI_NetPoint& netpoint) noexcept;
        CmdBoardNetPointAdd(Board& board, BoardLayer& layer, NetSignal& netsignal,
                            const Point& position) noexcept;
        CmdBoardNetPointAdd(Board& board, BoardLayer& layer, NetSignal& netsignal,
                            BI_FootprintPad& pad) noexcept;
        CmdBoardNetPointAdd(Board& board, BoardLayer& layer, NetSignal& netsignal,
                            BI_Via& via) noexcept;
        ~CmdBoardNetPointAdd() noexcept;

        // Getters
        BI_NetPoint* getNetPoint() const noexcept {return mNetPoint;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        Board& mBoard;
        BoardLayer* mLayer;
        NetSignal* mNetSignal;
        Point mPosition;
        BI_FootprintPad* mFootprintPad;
        BI_Via* mVia;
        BI_NetPoint* mNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDNETPOINTADD_H
