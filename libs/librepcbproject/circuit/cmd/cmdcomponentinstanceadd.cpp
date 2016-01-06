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
#include "cmdcomponentinstanceadd.h"
#include "../circuit.h"
#include "../componentinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceAdd::CmdComponentInstanceAdd(Circuit& circuit, const library::Component& cmp,
                                     const library::ComponentSymbolVariant& symbVar, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component"), parent),
    mCircuit(circuit), mComponent(cmp), mSymbVar(symbVar), mComponentInstance(nullptr)
{
}

CmdComponentInstanceAdd::~CmdComponentInstanceAdd() noexcept
{
    if (!isExecuted())
        delete mComponentInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdComponentInstanceAdd::redo() throw (Exception)
{
    if (!mComponentInstance) // only the first time
    {
        mComponentInstance = mCircuit.createComponentInstance(mComponent, mSymbVar); // throws an exception on error
    }

    mCircuit.addComponentInstance(*mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mCircuit.removeComponentInstance(*mComponentInstance);
        throw;
    }
}

void CmdComponentInstanceAdd::undo() throw (Exception)
{
    mCircuit.removeComponentInstance(*mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.addComponentInstance(*mComponentInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
