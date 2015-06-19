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
#include "cmdsymbolinstanceedit.h"
#include "../items/si_symbol.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceEdit::CmdSymbolInstanceEdit(SI_Symbol& symbol, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit symbol instance"), parent), mSymbol(symbol),
    mOldPos(symbol.getPosition()), mNewPos(mOldPos),
    mOldRotation(symbol.getAngle()), mNewRotation(mOldRotation)
{
}

CmdSymbolInstanceEdit::~CmdSymbolInstanceEdit() noexcept
{
    if ((mRedoCount == 0) && (mUndoCount == 0))
    {
        mSymbol.setPosition(mOldPos);
        mSymbol.setAngle(mOldRotation);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSymbolInstanceEdit::setPosition(Point& pos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = pos;
    if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = mOldPos + deltaPos;
    if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewRotation = angle;
    if (immediate) mSymbol.setAngle(mNewRotation);
}

void CmdSymbolInstanceEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos.rotate(angle, center);
    mNewRotation += angle;
    if (immediate)
    {
        mSymbol.setPosition(mNewPos);
        mSymbol.setAngle(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSymbolInstanceEdit::redo() throw (Exception)
{
    try
    {
        mSymbol.setPosition(mNewPos);
        mSymbol.setAngle(mNewRotation);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mSymbol.setPosition(mOldPos);
        mSymbol.setAngle(mOldRotation);
        throw;
    }
}

void CmdSymbolInstanceEdit::undo() throw (Exception)
{
    try
    {
        mSymbol.setPosition(mOldPos);
        mSymbol.setAngle(mOldRotation);
        UndoCommand::undo();
    }
    catch (Exception &e)
    {
        mSymbol.setPosition(mNewPos);
        mSymbol.setAngle(mNewRotation);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
