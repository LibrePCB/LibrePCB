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
#include "cmdnetclasssetname.h"
#include "../netclass.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetClassSetName::CmdNetClassSetName(NetClass& netclass, const QString& newName,
                               UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Rename netclass"), parent),
    mNetClass(netclass), mOldName(netclass.getName()), mNewName(newName)
{
}

CmdNetClassSetName::~CmdNetClassSetName() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetClassSetName::redo() throw (Exception)
{
    mNetClass.setName(mNewName); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mNetClass.setName(mOldName);
        throw;
    }
}

void CmdNetClassSetName::undo() throw (Exception)
{
    mNetClass.setName(mOldName); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mNetClass.setName(mNewName);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
