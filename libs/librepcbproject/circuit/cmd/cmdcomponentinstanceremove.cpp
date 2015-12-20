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
#include "cmdcomponentinstanceremove.h"
#include "../circuit.h"
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceRemove::CmdComponentInstanceRemove(Circuit& circuit,
                                                       ComponentInstance& cmp,
                                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove generic component"), parent),
    mCircuit(circuit), mComponentInstance(cmp)
{
}

CmdComponentInstanceRemove::~CmdComponentInstanceRemove() noexcept
{
    if (isExecuted())
        delete &mComponentInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdComponentInstanceRemove::redo() throw (Exception)
{
    mCircuit.removeComponentInstance(mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mCircuit.addComponentInstance(mComponentInstance);
        throw;
    }
}

void CmdComponentInstanceRemove::undo() throw (Exception)
{
    mCircuit.addComponentInstance(mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.removeComponentInstance(mComponentInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
