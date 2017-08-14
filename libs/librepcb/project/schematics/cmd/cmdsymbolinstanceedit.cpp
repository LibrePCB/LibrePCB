/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceEdit::CmdSymbolInstanceEdit(SI_Symbol& symbol) noexcept :
    UndoCommand(tr("Edit symbol instance")), mSymbol(symbol),
    mOldPos(symbol.getPosition()), mNewPos(mOldPos),
    mOldRotation(symbol.getRotation()), mNewRotation(mOldRotation)
{
}

CmdSymbolInstanceEdit::~CmdSymbolInstanceEdit() noexcept
{
    if (!wasEverExecuted()) {
        mSymbol.setPosition(mOldPos);
        mSymbol.setRotation(mOldRotation);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSymbolInstanceEdit::setPosition(Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = pos;
    if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = mOldPos + deltaPos;
    if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRotation = angle;
    if (immediate) mSymbol.setRotation(mNewRotation);
}

void CmdSymbolInstanceEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos.rotate(angle, center);
    mNewRotation += angle;
    if (immediate)
    {
        mSymbol.setPosition(mNewPos);
        mSymbol.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdSymbolInstanceEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewPos != mOldPos)             return true;
    if (mNewRotation != mOldRotation)   return true;
    return false;
}

void CmdSymbolInstanceEdit::performUndo()
{
    mSymbol.setPosition(mOldPos);
    mSymbol.setRotation(mOldRotation);
}

void CmdSymbolInstanceEdit::performRedo()
{
    mSymbol.setPosition(mNewPos);
    mSymbol.setRotation(mNewRotation);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
