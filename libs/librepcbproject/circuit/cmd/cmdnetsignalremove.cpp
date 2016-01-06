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

CmdNetSignalRemove::CmdNetSignalRemove(Circuit& circuit, NetSignal& netsignal,
                                     UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove netsignal"), parent),
    mCircuit(circuit), mNetSignal(netsignal)
{
}

CmdNetSignalRemove::~CmdNetSignalRemove() noexcept
{
    if (isExecuted())
        delete &mNetSignal;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetSignalRemove::redo() throw (Exception)
{
    mCircuit.removeNetSignal(mNetSignal); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.addNetSignal(mNetSignal);
        throw;
    }
}

void CmdNetSignalRemove::undo() throw (Exception)
{
    mCircuit.addNetSignal(mNetSignal); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.removeNetSignal(mNetSignal);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
