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
#include "cmdgencompinstsetname.h"
#include "../circuit.h"
#include "../gencompinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdGenCompInstSetName::CmdGenCompInstSetName(Circuit& circuit, GenCompInstance& genComp,
                                             const QString& newName, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Change Component Name"), parent),
    mCircuit(circuit), mGenCompInstance(genComp), mOldName(genComp.getName()), mNewName(newName)
{
}

CmdGenCompInstSetName::~CmdGenCompInstSetName() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdGenCompInstSetName::redo() throw (Exception)
{
    mCircuit.setGenCompInstanceName(mGenCompInstance, mNewName); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mOldName);
        throw;
    }
}

void CmdGenCompInstSetName::undo() throw (Exception)
{
    mCircuit.setGenCompInstanceName(mGenCompInstance, mOldName); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mNewName);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
