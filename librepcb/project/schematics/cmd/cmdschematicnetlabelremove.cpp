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
#include "cmdschematicnetlabelremove.h"
#include "../schematic.h"
#include "../items/si_netlabel.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLabelRemove::CmdSchematicNetLabelRemove(Schematic& schematic,
                                                       SI_NetLabel& netlabel,
                                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove netlabel"), parent),
    mSchematic(schematic), mNetLabel(netlabel)
{
}

CmdSchematicNetLabelRemove::~CmdSchematicNetLabelRemove() noexcept
{
    if (isExecuted())
        delete &mNetLabel;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetLabelRemove::redo() throw (Exception)
{
    mSchematic.removeNetLabel(mNetLabel); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.addNetLabel(mNetLabel);
        throw;
    }
}

void CmdSchematicNetLabelRemove::undo() throw (Exception)
{
    mSchematic.addNetLabel(mNetLabel); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.removeNetLabel(mNetLabel);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
