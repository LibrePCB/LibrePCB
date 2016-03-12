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
#include "cmdnetsignalremove.h"
#include "../netsignal.h"
#include "../circuit.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetSignalRemove::CmdNetSignalRemove(Circuit& circuit, NetSignal& netsignal) noexcept :
    UndoCommand(tr("Remove netsignal")),
    mCircuit(circuit), mNetSignal(netsignal)
{
}

CmdNetSignalRemove::~CmdNetSignalRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdNetSignalRemove::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true;
}

void CmdNetSignalRemove::performUndo() throw (Exception)
{
    mCircuit.addNetSignal(mNetSignal); // can throw
}

void CmdNetSignalRemove::performRedo() throw (Exception)
{
    mCircuit.removeNetSignal(mNetSignal); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
