/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "cmdnetsignaladd.h"
#include "../netsignal.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetSignalAdd::CmdNetSignalAdd(Circuit& circuit, const QUuid& netclass,
                                 const QString& name, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add netsignal"), parent),
    mCircuit(circuit), mNetClass(netclass), mName(name), mNetSignal(0)
{
}

CmdNetSignalAdd::~CmdNetSignalAdd() noexcept
{
    if (!mIsExecuted)
        delete mNetSignal;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetSignalAdd::redo() throw (Exception)
{
    if (!mNetSignal) // only the first time
        mNetSignal = mCircuit.createNetSignal(mNetClass, mName); // throws an exception on error

    mCircuit.addNetSignal(mNetSignal); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.removeNetSignal(mNetSignal);
        throw;
    }
}

void CmdNetSignalAdd::undo() throw (Exception)
{
    mCircuit.removeNetSignal(mNetSignal); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.addNetSignal(mNetSignal);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
