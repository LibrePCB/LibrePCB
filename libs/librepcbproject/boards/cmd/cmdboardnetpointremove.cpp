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
#include "cmdboardnetpointremove.h"
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

CmdBoardNetPointRemove::CmdBoardNetPointRemove(BI_NetPoint& netpoint) noexcept :
    UndoCommand(tr("Remove netpoint")),
    mBoard(netpoint.getBoard()), mNetPoint(netpoint)
{
}

CmdBoardNetPointRemove::~CmdBoardNetPointRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetPointRemove::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true;
}

void CmdBoardNetPointRemove::performUndo() throw (Exception)
{
    mBoard.addNetPoint(mNetPoint); // can throw
}

void CmdBoardNetPointRemove::performRedo() throw (Exception)
{
    mBoard.removeNetPoint(mNetPoint); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
