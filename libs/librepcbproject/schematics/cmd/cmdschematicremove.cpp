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
#include "cmdschematicremove.h"
#include "../schematic.h"
#include "../../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicRemove::CmdSchematicRemove(Project& project, Schematic* schematic,
                                       UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove schematic"), parent),
    mProject(project), mSchematic(schematic), mPageIndex(-1)
{
}

CmdSchematicRemove::~CmdSchematicRemove() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicRemove::redo() throw (Exception)
{
    mPageIndex = mProject.getSchematicIndex(mSchematic);
    mProject.removeSchematic(mSchematic); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mProject.addSchematic(mSchematic, mPageIndex);
        throw;
    }
}

void CmdSchematicRemove::undo() throw (Exception)
{
    mProject.addSchematic(mSchematic, mPageIndex); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mProject.removeSchematic(mSchematic);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
