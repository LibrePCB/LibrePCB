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
#include "cmdgencompinstadd.h"
#include "../circuit.h"
#include "../gencompinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdGenCompInstAdd::CmdGenCompInstAdd(Circuit& circuit, const library::Component& genComp,
                                     const library::ComponentSymbolVariant& symbVar, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add generic component"), parent),
    mCircuit(circuit), mGenComp(genComp), mSymbVar(symbVar), mGenCompInstance(nullptr)
{
}

CmdGenCompInstAdd::~CmdGenCompInstAdd() noexcept
{
    if (!isExecuted())
        delete mGenCompInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdGenCompInstAdd::redo() throw (Exception)
{
    if (!mGenCompInstance) // only the first time
    {
        mGenCompInstance = mCircuit.createGenCompInstance(mGenComp, mSymbVar); // throws an exception on error
    }

    mCircuit.addGenCompInstance(*mGenCompInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mCircuit.removeGenCompInstance(*mGenCompInstance);
        throw;
    }
}

void CmdGenCompInstAdd::undo() throw (Exception)
{
    mCircuit.removeGenCompInstance(*mGenCompInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.addGenCompInstance(*mGenCompInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
