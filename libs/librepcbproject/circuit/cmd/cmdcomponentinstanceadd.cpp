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
#include "../../project.h"
#include "../../library/projectlibrary.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceAdd::CmdComponentInstanceAdd(Circuit& circuit, const Uuid& cmp,
                                                 const Uuid& symbVar, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component"), parent),
    mCircuit(circuit), mComponentUuid(cmp), mSymbVarUuid(symbVar), mComponentInstance(nullptr)
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
        library::Component* cmp = mCircuit.getProject().getLibrary().getComponent(mComponentUuid);
        if (!cmp) {
            throw RuntimeError(__FILE__, __LINE__, mComponentUuid.toStr(),
                QString(tr("The component with the UUID \"%1\" does not exist in the "
                "project's library!")).arg(mComponentUuid.toStr()));
        }
        mComponentInstance = mCircuit.createComponentInstance(*cmp, mSymbVarUuid); // throws an exception on error
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
