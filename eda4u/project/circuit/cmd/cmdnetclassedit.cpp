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
#include "cmdnetclassedit.h"
#include "../netclass.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetClassEdit::CmdNetClassEdit(Circuit& circuit, NetClass& netclass, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit netclass"), parent), mCircuit(circuit), mNetClass(netclass),
    mOldName(netclass.getName()), mNewName(mOldName)
{
}

CmdNetClassEdit::~CmdNetClassEdit() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdNetClassEdit::setName(const QString& name) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewName = name;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetClassEdit::redo() throw (Exception)
{
    try
    {
        mCircuit.setNetClassName(mNetClass, mNewName);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mCircuit.setNetClassName(mNetClass, mOldName);
        throw;
    }
}

void CmdNetClassEdit::undo() throw (Exception)
{
    try
    {
        mCircuit.setNetClassName(mNetClass, mOldName);
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mCircuit.setNetClassName(mNetClass, mNewName);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
