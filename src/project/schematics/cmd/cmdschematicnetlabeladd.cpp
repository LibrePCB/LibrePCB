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
#include "cmdschematicnetlabeladd.h"
#include "../schematic.h"
#include "../schematicnetlabel.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLabelAdd::CmdSchematicNetLabelAdd(Schematic& schematic, NetSignal& netsignal,
                                                 const Point& position, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add netlabel"), parent),
    mSchematic(schematic), mNetSignal(&netsignal), mPosition(position), mNetLabel(nullptr)
{
}

CmdSchematicNetLabelAdd::~CmdSchematicNetLabelAdd() noexcept
{
    if ((mNetLabel) && (!isExecuted()))
        delete mNetLabel;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetLabelAdd::redo() throw (Exception)
{
    if (!mNetLabel) // only the first time
    {
        mNetLabel = mSchematic.createNetLabel(*mNetSignal, mPosition); // throws an exception on error
    }

    mSchematic.addNetLabel(*mNetLabel); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mSchematic.removeNetLabel(*mNetLabel);
        throw;
    }
}

void CmdSchematicNetLabelAdd::undo() throw (Exception)
{
    mSchematic.removeNetLabel(*mNetLabel); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mSchematic.addNetLabel(*mNetLabel);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
