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
#include "cmdboardviaadd.h"
#include "../board.h"
#include "../items/bi_via.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardViaAdd::CmdBoardViaAdd(BI_Via& via) noexcept :
    UndoCommand(tr("Add via")),
    mBoard(via.getBoard()), mPosition(), mShape(), mSize(), mDrillDiameter(),
    mNetSignal(nullptr), mVia(&via)
{
}

CmdBoardViaAdd::CmdBoardViaAdd(Board& board, const Point& position, BI_Via::Shape shape,
        const Length& size, const Length& drillDiameter, NetSignal* netsignal) noexcept :
    UndoCommand(tr("Add via")),
    mBoard(board), mPosition(position), mShape(shape), mSize(size),
    mDrillDiameter(drillDiameter), mNetSignal(netsignal), mVia(nullptr)
{
}

CmdBoardViaAdd::~CmdBoardViaAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardViaAdd::performExecute() throw (Exception)
{
    if (!mVia) {
        // create new via
        mVia = new BI_Via(mBoard, mPosition, mShape, mSize, mDrillDiameter, mNetSignal);
    }

    performRedo(); // can throw

    return true;
}

void CmdBoardViaAdd::performUndo() throw (Exception)
{
    mBoard.removeVia(*mVia); // can throw
}

void CmdBoardViaAdd::performRedo() throw (Exception)
{
    mBoard.addVia(*mVia); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
