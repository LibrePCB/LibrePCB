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
#include "undocommandgroup.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UndoCommandGroup::UndoCommandGroup(const QString& text) noexcept :
    UndoCommand(text)
{
}

UndoCommandGroup::~UndoCommandGroup() noexcept
{
    // delete childs in reverse order
    while (mChilds.count() > 0) {
        delete mChilds.takeLast();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void UndoCommandGroup::appendChild(UndoCommand* cmd) throw (Exception)
{
    // make sure "cmd" is deleted when going out of scope (e.g. because of an exception)
    QScopedPointer<UndoCommand> cmdScopeGuard(cmd);

    if ((!cmd) || (mChilds.contains(cmd)) || (wasEverReverted())) {
        throw LogicError(__FILE__, __LINE__);
    }

    if (wasEverExecuted()) {
        cmd->execute(); // can throw
    }

    mChilds.append(cmdScopeGuard.take());
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool UndoCommandGroup::performExecute() throw (Exception)
{
    // TODO: use scope guard

    int i;

    try
    {
        for (i = 0; i < mChilds.count(); i++) { // from bottom to top
            mChilds[i]->execute();
        }
    }
    catch (Exception&)
    {
        try
        {
            // could not execute the child with index "i"!
            // try to revert the whole action --> undo the executed commands
            for (; i >= 0; i--) { // from "i" to bottom
                mChilds[i]->undo();
            }
        }
        catch (Exception&)
        {
            qFatal("UndoCommandGroup: Internal Fatal Error");
        }
        throw;
    }

    return (mChilds.count() > 0);
}

void UndoCommandGroup::performUndo() throw (Exception)
{
    // TODO: use scope guard

    int i;

    try
    {
        for (i = mChilds.count()-1; i >= 0; i--) { // from top to bottom
            mChilds[i]->undo();
        }
    }
    catch (Exception&)
    {
        try
        {
            // could not undo the child with index "i"!
            // try to revert the whole action --> redo the undoed commands
            for (; i < mChilds.count(); i++) { // from "i" to top
                mChilds[i]->redo();
            }
        }
        catch (Exception&)
        {
            qFatal("UndoCommandGroup: Internal Fatal Error");
        }
        throw;
    }
}

void UndoCommandGroup::performRedo() throw (Exception)
{
    // TODO: use scope guard

    int i;

    try
    {
        for (i = 0; i < mChilds.count(); i++) { // from bottom to top
            mChilds[i]->redo();
        }
    }
    catch (Exception&)
    {
        try
        {
            // could not undo the child with index "i"!
            // try to revert the whole action --> undo the redoed commands
            for (; i >= 0; i--) { // from "i" to bottom
                mChilds[i]->undo();
            }
        }
        catch (Exception&)
        {
            qFatal("UndoCommandGroup: Internal Fatal Error");
        }
        throw;
    }
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

void UndoCommandGroup::execNewChildCmd(UndoCommand* cmd) throw (Exception)
{
    QScopedPointer<UndoCommand> cmdScopeGuard(cmd);

    if ((!cmd) || (mChilds.contains(cmd)) || (wasEverExecuted())) {
        throw LogicError(__FILE__, __LINE__);
    }

    cmdScopeGuard->execute(); // can throw
    mChilds.append(cmdScopeGuard.take());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
