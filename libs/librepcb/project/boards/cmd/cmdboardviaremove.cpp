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
#include "cmdboardviaremove.h"
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

CmdBoardViaRemove::CmdBoardViaRemove(BI_Via& via) noexcept :
    UndoCommand(tr("Remove via")),
    mBoard(via.getBoard()), mVia(via)
{
}

CmdBoardViaRemove::~CmdBoardViaRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardViaRemove::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true;
}

void CmdBoardViaRemove::performUndo() throw (Exception)
{
    mBoard.addVia(mVia); // can throw
}

void CmdBoardViaRemove::performRedo() throw (Exception)
{
    mBoard.removeVia(mVia); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
