/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "cmdcomponentinstanceedit.h"
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

CmdComponentInstanceEdit::CmdComponentInstanceEdit(Circuit& circuit, ComponentInstance& cmp) noexcept :
    UndoCommand(tr("Edit Component")), mCircuit(circuit), mComponentInstance(cmp),
    mOldName(cmp.getName()), mNewName(mOldName),
    mOldValue(cmp.getValue()), mNewValue(mOldValue)
{
}

CmdComponentInstanceEdit::~CmdComponentInstanceEdit() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdComponentInstanceEdit::setName(const QString& name) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewName = name;
}

void CmdComponentInstanceEdit::setValue(const QString& value) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewValue = value;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdComponentInstanceEdit::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true; // TODO: determine if the component was really modified
}

void CmdComponentInstanceEdit::performUndo() throw (Exception)
{
    mCircuit.setComponentInstanceName(mComponentInstance, mOldName); // can throw
    mComponentInstance.setValue(mOldValue);
}

void CmdComponentInstanceEdit::performRedo() throw (Exception)
{
    mCircuit.setComponentInstanceName(mComponentInstance, mNewName); // can throw
    mComponentInstance.setValue(mNewValue);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
