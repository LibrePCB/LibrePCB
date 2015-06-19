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
#include "cmdboardadd.h"
#include "../board.h"
#include "../../project.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardAdd::CmdBoardAdd(Project& project, const QString& name,
                         UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add board"), parent),
    mProject(project), mName(name), mBoard(nullptr), mPageIndex(-1)
{
}

CmdBoardAdd::~CmdBoardAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdBoardAdd::redo() throw (Exception)
{
    if (!mBoard) // only the first time
        mBoard = mProject.createBoard(mName); // throws an exception on error

    mProject.addBoard(mBoard, mPageIndex); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mProject.removeBoard(mBoard);
        throw;
    }
}

void CmdBoardAdd::undo() throw (Exception)
{
    mProject.removeBoard(mBoard); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mProject.addBoard(mBoard, mPageIndex);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
