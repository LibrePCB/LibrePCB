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
#include "cmdnetclassremove.h"
#include "../netclass.h"
#include "../circuit.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetClassRemove::CmdNetClassRemove(Circuit& circuit, NetClass& netclass,
                                     UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove netclass"), parent),
    mCircuit(circuit), mNetClass(netclass)
{
}

CmdNetClassRemove::~CmdNetClassRemove() noexcept
{
    if (isExecuted())
        delete &mNetClass;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetClassRemove::redo() throw (Exception)
{
    mCircuit.removeNetClass(mNetClass); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.addNetClass(mNetClass);
        throw;
    }
}

void CmdNetClassRemove::undo() throw (Exception)
{
    mCircuit.addNetClass(mNetClass); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mCircuit.removeNetClass(mNetClass);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
