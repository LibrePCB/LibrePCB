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
#include "cmdgencompinstsetvalue.h"
#include "../gencompinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdGenCompInstSetValue::CmdGenCompInstSetValue(GenCompInstance& genComp, const QString& newValue,
                                               UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Change Component Value"), parent),
    mGenCompInstance(genComp), mOldValue(genComp.getValue()), mNewValue(newValue)
{
}

CmdGenCompInstSetValue::~CmdGenCompInstSetValue() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdGenCompInstSetValue::redo() throw (Exception)
{
    mGenCompInstance.setValue(mNewValue);

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mGenCompInstance.setValue(mOldValue);
        throw;
    }
}

void CmdGenCompInstSetValue::undo() throw (Exception)
{
    mGenCompInstance.setValue(mOldValue);

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mGenCompInstance.setValue(mNewValue);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
