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
#include "cmdnetclassadd.h"
#include "../netclass.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetClassAdd::CmdNetClassAdd(Circuit& circuit, const QString& name,
                               UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add netclass"), parent),
    mCircuit(circuit), mName(name), mNetClass(nullptr)
{
}

CmdNetClassAdd::~CmdNetClassAdd() noexcept
{
    if (!isExecuted())
        delete mNetClass;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetClassAdd::redo() throw (Exception)
{
    if (!mNetClass) // only the first time
        mNetClass = new NetClass(mCircuit, mName); // throws an exception on error

    mCircuit.addNetClass(*mNetClass); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mCircuit.removeNetClass(*mNetClass);
        throw;
    }
}

void CmdNetClassAdd::undo() throw (Exception)
{
    mCircuit.removeNetClass(*mNetClass); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.addNetClass(*mNetClass);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
