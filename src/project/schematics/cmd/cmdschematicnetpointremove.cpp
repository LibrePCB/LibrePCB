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
#include "cmdschematicnetpointremove.h"
#include "../schematic.h"
#include "../schematicnetpoint.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetPointRemove::CmdSchematicNetPointRemove(Schematic& schematic,
                                                       SchematicNetPoint* netpoint,
                                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove netpoint"), parent),
    mSchematic(schematic), mNetPoint(netpoint)
{
}

CmdSchematicNetPointRemove::~CmdSchematicNetPointRemove() noexcept
{
    if (mIsExecuted)
        delete mNetPoint;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetPointRemove::redo() throw (Exception)
{
    mSchematic.removeNetPoint(mNetPoint); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.addNetPoint(mNetPoint);
        throw;
    }
}

void CmdSchematicNetPointRemove::undo() throw (Exception)
{
    mSchematic.addNetPoint(mNetPoint); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.removeNetPoint(mNetPoint);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
