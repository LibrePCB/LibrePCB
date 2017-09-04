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
#include "cmdcomponentsignaledit.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentSignalEdit::CmdComponentSignalEdit(ComponentSignal& signal) noexcept :
    UndoCommand(tr("Edit component signal")), mSignal(signal),
    mOldName(signal.getName()), mNewName(mOldName),
    mOldRole(signal.getRole()), mNewRole(mOldRole),
    mOldForcedNetName(signal.getForcedNetName()), mNewForcedNetName(mOldForcedNetName),
    mOldIsRequired(signal.isRequired()), mNewIsRequired(mOldIsRequired),
    mOldIsNegated(signal.isNegated()), mNewIsNegated(mOldIsNegated),
    mOldIsClock(signal.isClock()), mNewIsClock(mOldIsClock)
{
}

CmdComponentSignalEdit::~CmdComponentSignalEdit() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdComponentSignalEdit::setName(const QString& name) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewName = name;
}

void CmdComponentSignalEdit::setRole(const SignalRole& role) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRole = role;
}

void CmdComponentSignalEdit::setForcedNetName(const QString& name) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewForcedNetName = name;
}

void CmdComponentSignalEdit::setIsRequired(bool required) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsRequired = required;
}

void CmdComponentSignalEdit::setIsNegated(bool negated) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsNegated = negated;
}

void CmdComponentSignalEdit::setIsClock(bool clock) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsClock = clock;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdComponentSignalEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewName != mOldName)                       return true;
    if (mNewRole != mOldRole)                       return true;
    if (mNewForcedNetName != mOldForcedNetName)     return true;
    if (mNewIsRequired != mOldIsRequired)           return true;
    if (mNewIsNegated != mOldIsNegated)             return true;
    if (mNewIsClock != mOldIsClock)                 return true;
    return false;
}

void CmdComponentSignalEdit::performUndo()
{
    mSignal.setName(mOldName);
    mSignal.setRole(mOldRole);
    mSignal.setForcedNetName(mOldForcedNetName);
    mSignal.setIsRequired(mOldIsRequired);
    mSignal.setIsNegated(mOldIsNegated);
    mSignal.setIsClock(mOldIsClock);
}

void CmdComponentSignalEdit::performRedo()
{
    mSignal.setName(mNewName);
    mSignal.setRole(mNewRole);
    mSignal.setForcedNetName(mNewForcedNetName);
    mSignal.setIsRequired(mNewIsRequired);
    mSignal.setIsNegated(mNewIsNegated);
    mSignal.setIsClock(mNewIsClock);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
