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
#include "undocommand.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UndoCommand::UndoCommand(const QString& text) noexcept :
    mText(text), mRedoCount(0), mUndoCount(0)
{
}

UndoCommand::~UndoCommand() noexcept
{
    Q_ASSERT(qAbs(mRedoCount - mUndoCount) <= 1);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void UndoCommand::execute() throw (Exception)
{
    if (wasEverExecuted()) {
        throw LogicError(__FILE__, __LINE__);
    }

    performExecute(); // can throw
    mRedoCount++;
}

void UndoCommand::undo() throw (Exception)
{
    if (!isCurrentlyExecuted()) {
        throw LogicError(__FILE__, __LINE__);
    }

    performUndo(); // can throw
    mUndoCount++;
}

void UndoCommand::redo() throw (Exception)
{
    if ((!wasEverExecuted()) || (isCurrentlyExecuted())) {
        throw LogicError(__FILE__, __LINE__);
    }

    performRedo(); // can throw
    mRedoCount++;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
