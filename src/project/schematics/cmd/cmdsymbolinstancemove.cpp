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
#include "cmdsymbolinstancemove.h"
#include "../symbolinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceMove::CmdSymbolInstanceMove(SymbolInstance& symbol, UndoCommand* parent) throw (Exception) :
    UndoCommand(QCoreApplication::translate("CmdSchematicNetLineAdd", "Add netline"), parent),
    mSymbolInstance(symbol), mStartPos(symbol.getPosition()), mDeltaPos(0, 0),
    mEndPos(symbol.getPosition()), mRedoOrUndoCalled(false)
{
}

CmdSymbolInstanceMove::~CmdSymbolInstanceMove() noexcept
{
    if ((!mRedoOrUndoCalled) && (mDeltaPos != Point()))
        mSymbolInstance.setPosition(mStartPos, false);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSymbolInstanceMove::setDeltaToStartPosTemporary(Point& deltaPos) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = deltaPos;
    mSymbolInstance.setPosition(mStartPos + mDeltaPos, false);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSymbolInstanceMove::redo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::redo(); // throws an exception on error
    mEndPos = mStartPos + mDeltaPos;
    mSymbolInstance.setPosition(mEndPos);
}

void CmdSymbolInstanceMove::undo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::undo();
    mSymbolInstance.setPosition(mStartPos);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
