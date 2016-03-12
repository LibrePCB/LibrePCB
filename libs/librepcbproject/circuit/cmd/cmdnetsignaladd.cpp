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
#include "cmdnetsignaladd.h"
#include "../netsignal.h"
#include "../circuit.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetSignalAdd::CmdNetSignalAdd(Circuit& circuit, NetClass& netclass, const QString& name) noexcept :
    UndoCommand(tr("Add netsignal")),
    mCircuit(circuit), mNetClass(netclass), mName(name), mNetSignal(nullptr)
{
}

CmdNetSignalAdd::~CmdNetSignalAdd() noexcept
{
    if (!isCurrentlyExecuted())
        delete mNetSignal;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdNetSignalAdd::performExecute() throw (Exception)
{
    mNetSignal = mCircuit.createNetSignal(mNetClass, mName); // can throw

    performRedo(); // can throw
}

void CmdNetSignalAdd::performUndo() throw (Exception)
{
    mCircuit.removeNetSignal(*mNetSignal); // can throw
}

void CmdNetSignalAdd::performRedo() throw (Exception)
{
    mCircuit.addNetSignal(*mNetSignal); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
