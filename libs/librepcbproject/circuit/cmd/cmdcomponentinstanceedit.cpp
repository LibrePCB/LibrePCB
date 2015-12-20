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
#include "cmdcomponentinstanceedit.h"
#include "../circuit.h"
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceEdit::CmdComponentInstanceEdit(Circuit& circuit, ComponentInstance& cmp,
                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit Component"), parent), mCircuit(circuit), mComponentInstance(cmp),
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
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewName = name;
}

void CmdComponentInstanceEdit::setValue(const QString& value) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewValue = value;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdComponentInstanceEdit::redo() throw (Exception)
{
    try
    {
        mCircuit.setComponentInstanceName(mComponentInstance, mNewName);
        mComponentInstance.setValue(mNewValue);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mCircuit.setComponentInstanceName(mComponentInstance, mOldName);
        mComponentInstance.setValue(mOldValue);
        throw;
    }
}

void CmdComponentInstanceEdit::undo() throw (Exception)
{
    try
    {
        mCircuit.setComponentInstanceName(mComponentInstance, mOldName);
        mComponentInstance.setValue(mOldValue);
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.setComponentInstanceName(mComponentInstance, mNewName);
        mComponentInstance.setValue(mNewValue);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
