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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdboardnetlineadd.h"
#include "../board.h"
#include "../items/bi_netline.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardNetLineAdd::CmdBoardNetLineAdd(BI_NetLine& netline) noexcept :
    UndoCommand(tr("Add board trace")),
    mBoard(netline.getBoard()), mStartPoint(netline.getStartPoint()),
    mEndPoint(netline.getEndPoint()), mNetLine(&netline)
{
}

CmdBoardNetLineAdd::CmdBoardNetLineAdd(Board& board, BI_NetPoint& startPoint,
                                       BI_NetPoint& endPoint, const Length& width) noexcept :
    UndoCommand(tr("Add board trace")),
    mBoard(board), mStartPoint(startPoint), mEndPoint(endPoint), mWidth(width),
    mNetLine(nullptr)
{
}

CmdBoardNetLineAdd::~CmdBoardNetLineAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetLineAdd::performExecute() throw (Exception)
{
    if (!mNetLine) {
        // create new netline
        mNetLine = new BI_NetLine(mBoard, mStartPoint, mEndPoint, mWidth); // can throw
    }

    performRedo(); // can throw

    return true;
}

void CmdBoardNetLineAdd::performUndo() throw (Exception)
{
    mBoard.removeNetLine(*mNetLine); // can throw
}

void CmdBoardNetLineAdd::performRedo() throw (Exception)
{
    mBoard.addNetLine(*mNetLine); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
