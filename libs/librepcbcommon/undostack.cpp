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
#include <QtWidgets>
#include "undostack.h"
#include "exceptions.h"
#include "undocommand.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UndoStack::UndoStack() noexcept :
    QObject(0), mCurrentIndex(0), mCleanIndex(0), mCommandActive(false)
{
}

UndoStack::~UndoStack() noexcept
{
    clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString UndoStack::getUndoText() const noexcept
{
    if (canUndo())
        return QString(tr("Undo: %1")).arg(mCommands[mCurrentIndex-1]->getText());
    else
        return tr("Undo");
}

QString UndoStack::getRedoText() const noexcept
{
    if (canRedo())
        return QString(tr("Redo: %1")).arg(mCommands[mCurrentIndex]->getText());
    else
        return tr("Redo");
}

bool UndoStack::canUndo() const noexcept
{
    return (mCurrentIndex > 0);
}

bool UndoStack::canRedo() const noexcept
{
    return (mCurrentIndex < mCommands.count());
}

bool UndoStack::isClean() const noexcept
{
    return (mCurrentIndex == mCleanIndex);
}

bool UndoStack::isCommandActive() const noexcept
{
    return mCommandActive;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void UndoStack::setClean() noexcept
{
    if (isClean())
        return;

    mCleanIndex = mCurrentIndex;

    emit cleanChanged(true);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void UndoStack::execCmd(UndoCommand* cmd, bool autoMerge) throw (Exception)
{
    Q_CHECK_PTR(cmd);

    if (mCommandActive)
    {
        delete cmd;
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("Another command is active "
                           "at the moment. Please finish that command to continue."));
    }

    if (mCleanIndex > mCurrentIndex)
        mCleanIndex = -1; // the clean state will no longer exist -> make the index invalid

    // first, delete all commands above the current index (make redo impossible)
    // --> in reverse order (from top to bottom)!
    while (mCurrentIndex < mCommands.count())
        delete mCommands.takeLast();

    Q_ASSERT(mCurrentIndex == mCommands.count());

    try
    {
        cmd->redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        // delete the command because this UndoStack object will NOT take the ownership
        // over the failed command! and then rethrow the exception...
        delete cmd;
        throw;
    }

    // command successfully executed, try to merge it with the last executed command
    bool merged = false;
    if ((autoMerge) && (mCurrentIndex > 0))
        merged = mCommands[mCurrentIndex-1]->mergeWith(cmd); // try merging the commands

    if (!merged)
    {
        // command was not merged --> add it to the command stack and emit signals
        mCommands.append(cmd);
        mCurrentIndex++;

        // emit signals
        emit undoTextChanged(QString(tr("Undo: %1")).arg(cmd->getText()));
        emit redoTextChanged(tr("Redo"));
        emit canUndoChanged(true);
        emit canRedoChanged(false);
        emit cleanChanged(false);
    }
}

void UndoStack::beginCommand(const QString& text) throw (Exception)
{
    if (mCommandActive)
    {
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("Another command is active "
                           "at the moment. Please finish that command to continue."));
    }

    UndoCommand* cmd = new UndoCommand(text);
    execCmd(cmd, false); // throws an exception on error; emits all signals; does NOT merge
    mCommandActive = true;

    // emit signals
    emit canUndoChanged(false);
}

void UndoStack::appendToCommand(UndoCommand* cmd) throw (Exception)
{
    Q_ASSERT(cmd != nullptr);
    Q_ASSERT(mCurrentIndex == mCommands.count());

    if (!mCommandActive)
    {
        delete cmd;
        throw LogicError(__FILE__, __LINE__, QString(), tr("No command active!"));
    }

    try
    {
        cmd->redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        // delete the command because this UndoStack object will NOT take the ownership
        // over the failed command! and then rethrow the exception...
        delete cmd;
        throw;
    }

    mCommands.last()->appendChild(cmd); // append new command as a child of active command
}

void UndoStack::endCommand() throw (Exception)
{
    Q_ASSERT(mCurrentIndex == mCommands.count());

    if (!mCommandActive)
        throw LogicError(__FILE__, __LINE__, QString(), tr("No command active!"));

    if (mCommands.last()->getChildCount() == 0)
    {
        // the last command is empty --> remove it from the stack!
        abortCommand();
        return;
    }

    mCommandActive = false;

    // emit signals
    emit canUndoChanged(canUndo());
    emit commandEnded();
}

void UndoStack::abortCommand() throw (Exception)
{
    Q_ASSERT(mCurrentIndex == mCommands.count());

    if (!mCommandActive)
        throw LogicError(__FILE__, __LINE__, QString(), tr("No command active!"));

    mCommands.last()->undo(); // throws an exception on error
    mCurrentIndex--;
    mCommandActive = false;
    delete mCommands.takeLast(); // delete and remove the aborted command from the stack

    // emit signals
    emit undoTextChanged(getUndoText());
    emit redoTextChanged(tr("Redo"));
    emit canUndoChanged(canUndo());
    emit canRedoChanged(false);
    emit cleanChanged(isClean());
    emit commandAborted(); // this is important!
}

void UndoStack::undo() throw (Exception)
{
    if ((!canUndo()) || (mCommandActive)) // if a command is active, undo() is not allowed
        return;

    mCommands[mCurrentIndex-1]->undo(); // throws an exception on error
    mCurrentIndex--;

    // emit signals
    emit undoTextChanged(getUndoText());
    emit redoTextChanged(getRedoText());
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
    emit cleanChanged(isClean());
}

void UndoStack::redo() throw (Exception)
{
    if (!canRedo())
        return;

    mCommands[mCurrentIndex]->redo(); // throws an exception on error
    mCurrentIndex++;

    // emit signals
    emit undoTextChanged(getUndoText());
    emit redoTextChanged(getRedoText());
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
    emit cleanChanged(isClean());
}

void UndoStack::clear() noexcept
{
    if (mCommands.isEmpty())
        return;

    if (mCommandActive)
        try {abortCommand();} catch (...) {}

    // delete all commands in the stack from top to bottom (newest first, oldest last)!
    while (!mCommands.isEmpty())
        delete mCommands.takeLast();

    mCurrentIndex = 0;
    mCleanIndex = 0;
    mCommandActive = false;

    // emit signals
    emit undoTextChanged(tr("Undo"));
    emit redoTextChanged(tr("Redo"));
    emit canUndoChanged(false);
    emit canRedoChanged(false);
    emit cleanChanged(true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
