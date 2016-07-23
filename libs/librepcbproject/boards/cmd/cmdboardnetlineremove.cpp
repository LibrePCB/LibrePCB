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
#include "cmdboardnetlineremove.h"
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

CmdBoardNetLineRemove::CmdBoardNetLineRemove(BI_NetLine& netline) noexcept :
    UndoCommand(tr("Remove board trace")),
    mBoard(netline.getBoard()), mNetLine(netline)
{
}

CmdBoardNetLineRemove::~CmdBoardNetLineRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetLineRemove::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true;
}

void CmdBoardNetLineRemove::performUndo() throw (Exception)
{
    mBoard.addNetLine(mNetLine); // can throw
}

void CmdBoardNetLineRemove::performRedo() throw (Exception)
{
    mBoard.removeNetLine(mNetLine); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
