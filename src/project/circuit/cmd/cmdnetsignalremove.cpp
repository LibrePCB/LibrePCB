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
#include "cmdnetsignalremove.h"
#include "../netsignal.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetSignalRemove::CmdNetSignalRemove(Circuit& circuit, NetSignal* netsignal,
                                     QUndoCommand* parent) :
    QUndoCommand(QCoreApplication::translate("CmdNetSignalRemove", "Remove netsignal"), parent),
    mCircuit(circuit), mNetSignal(netsignal), mIsRemoved(false)
{
}

CmdNetSignalRemove::~CmdNetSignalRemove()
{
    if (mIsRemoved)
        delete mNetSignal;
}

/*****************************************************************************************
 *  Inherited from QUndoCommand
 ****************************************************************************************/

void CmdNetSignalRemove::redo()
{
    mCircuit.removeNetSignal(mNetSignal);
    mIsRemoved = true;
}

void CmdNetSignalRemove::undo()
{
    mCircuit.addNetSignal(mNetSignal);
    mIsRemoved = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
