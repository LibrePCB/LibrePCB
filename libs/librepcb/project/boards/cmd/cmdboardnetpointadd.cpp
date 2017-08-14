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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdboardnetpointadd.h"
#include "../board.h"
#include "../items/bi_netpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardNetPointAdd::CmdBoardNetPointAdd(BI_NetPoint& netpoint) noexcept :
    UndoCommand(tr("Add netpoint")),
    mBoard(netpoint.getBoard()), mLayer(nullptr), mNetSignal(nullptr), mPosition(),
    mFootprintPad(nullptr), mVia(nullptr), mNetPoint(&netpoint)
{
}

CmdBoardNetPointAdd::CmdBoardNetPointAdd(Board& board, BoardLayer& layer,
                                         NetSignal& netsignal, const Point& position) noexcept :
    UndoCommand(tr("Add netpoint")),
    mBoard(board), mLayer(&layer), mNetSignal(&netsignal), mPosition(position),
    mFootprintPad(nullptr), mVia(nullptr), mNetPoint(nullptr)
{
}

CmdBoardNetPointAdd::CmdBoardNetPointAdd(Board& board, BoardLayer& layer,
                                         NetSignal& netsignal, BI_FootprintPad& pad) noexcept :
    UndoCommand(tr("Add netpoint")),
    mBoard(board), mLayer(&layer), mNetSignal(&netsignal), mPosition(),
    mFootprintPad(&pad), mVia(nullptr), mNetPoint(nullptr)
{
}

CmdBoardNetPointAdd::CmdBoardNetPointAdd(Board& board, BoardLayer& layer,
                                         NetSignal& netsignal, BI_Via& via) noexcept :
    UndoCommand(tr("Add netpoint")),
    mBoard(board), mLayer(&layer), mNetSignal(&netsignal), mPosition(),
    mFootprintPad(nullptr), mVia(&via), mNetPoint(nullptr)
{
}

CmdBoardNetPointAdd::~CmdBoardNetPointAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetPointAdd::performExecute()
{
    if (!mNetPoint) {
        // create new netpoint
        if (mFootprintPad) {
            mNetPoint = new BI_NetPoint(mBoard, *mLayer, *mNetSignal, *mFootprintPad); // can throw
        } else if (mVia) {
            mNetPoint = new BI_NetPoint(mBoard, *mLayer, *mNetSignal, *mVia); // can throw
        } else {
            mNetPoint = new BI_NetPoint(mBoard, *mLayer, *mNetSignal, mPosition); // can throw
        }
    }

    performRedo(); // can throw

    return true;
}

void CmdBoardNetPointAdd::performUndo()
{
    mBoard.removeNetPoint(*mNetPoint); // can throw
}

void CmdBoardNetPointAdd::performRedo()
{
    mBoard.addNetPoint(*mNetPoint); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
