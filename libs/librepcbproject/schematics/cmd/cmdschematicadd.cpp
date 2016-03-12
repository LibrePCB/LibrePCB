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

CmdSchematicAdd::CmdSchematicAdd(Project& project, const QString& name) noexcept :
    UndoCommand(tr("Add schematic")),
    mProject(project), mName(name), mSchematic(nullptr), mPageIndex(-1)
{
}

CmdSchematicAdd::~CmdSchematicAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicAdd::performExecute() throw (Exception)
{
    mSchematic = mProject.createSchematic(mName); // can throw

    performRedo(); // can throw
}

void CmdSchematicAdd::performUndo() throw (Exception)
{
    mProject.removeSchematic(mSchematic); // can throw
}

void CmdSchematicAdd::performRedo() throw (Exception)
{
    mProject.addSchematic(mSchematic, mPageIndex); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
