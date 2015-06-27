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
#include "cmdgencompinstedit.h"
#include "../circuit.h"
#include "../gencompinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdGenCompInstEdit::CmdGenCompInstEdit(Circuit& circuit, GenCompInstance& genComp,
                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit Component"), parent), mCircuit(circuit), mGenCompInstance(genComp),
    mOldName(genComp.getName()), mNewName(mOldName),
    mOldValue(genComp.getValue()), mNewValue(mOldValue)
{
}

CmdGenCompInstEdit::~CmdGenCompInstEdit() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdGenCompInstEdit::setName(const QString& name) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewName = name;
}

void CmdGenCompInstEdit::setValue(const QString& value) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewValue = value;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdGenCompInstEdit::redo() throw (Exception)
{
    try
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mNewName);
        mGenCompInstance.setValue(mNewValue);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mOldName);
        mGenCompInstance.setValue(mOldValue);
        throw;
    }
}

void CmdGenCompInstEdit::undo() throw (Exception)
{
    try
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mOldName);
        mGenCompInstance.setValue(mOldValue);
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.setGenCompInstanceName(mGenCompInstance, mNewName);
        mGenCompInstance.setValue(mNewValue);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
