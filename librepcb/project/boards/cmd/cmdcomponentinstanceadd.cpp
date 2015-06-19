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
#include "cmdcomponentinstanceadd.h"
#include "../componentinstance.h"
#include "../board.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceAdd::CmdComponentInstanceAdd(Board& board, GenCompInstance& genComp,
                                                 const QUuid& componentUuid,
                                                 const Point& position, const Angle& rotation,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component to board"), parent),
    mBoard(board), mComponentInstance(nullptr)
{
    mComponentInstance = new ComponentInstance(board, genComp, componentUuid, position, rotation);
}

CmdComponentInstanceAdd::CmdComponentInstanceAdd(ComponentInstance& component,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component to board"), parent),
    mBoard(component.getBoard()), mComponentInstance(&component)
{
}

CmdComponentInstanceAdd::~CmdComponentInstanceAdd() noexcept
{
    if (!isExecuted())
        delete mComponentInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdComponentInstanceAdd::redo() throw (Exception)
{
    mBoard.addComponentInstance(*mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mBoard.removeComponentInstance(*mComponentInstance);
        throw;
    }
}

void CmdComponentInstanceAdd::undo() throw (Exception)
{
    mBoard.removeComponentInstance(*mComponentInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mBoard.addComponentInstance(*mComponentInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
