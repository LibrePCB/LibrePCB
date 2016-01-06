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
#include "cmdschematicadd.h"
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

CmdSchematicAdd::CmdSchematicAdd(Project& project, const QString& name,
                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add schematic"), parent),
    mProject(project), mName(name), mSchematic(nullptr), mPageIndex(-1)
{
}

CmdSchematicAdd::~CmdSchematicAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicAdd::redo() throw (Exception)
{
    if (!mSchematic) // only the first time
        mSchematic = mProject.createSchematic(mName); // throws an exception on error

    mProject.addSchematic(mSchematic, mPageIndex); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mProject.removeSchematic(mSchematic);
        throw;
    }
}

void CmdSchematicAdd::undo() throw (Exception)
{
    mProject.removeSchematic(mSchematic); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mProject.addSchematic(mSchematic, mPageIndex);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
