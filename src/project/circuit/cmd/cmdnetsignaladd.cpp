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
#include "cmdnetsignaladd.h"
#include "../netsignal.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetSignalAdd::CmdNetSignalAdd(Circuit& circuit, const QUuid& netclass, QUndoCommand* parent) :
    QUndoCommand(QCoreApplication::translate("CmdNetSignalAdd", "Add netsignal"), parent),
    mCircuit(circuit), mNetClass(netclass), mNetSignal(0), mIsAdded(false)
{
}

CmdNetSignalAdd::~CmdNetSignalAdd()
{
    if (!mIsAdded)
        delete mNetSignal;
}

/*****************************************************************************************
 *  Inherited from QUndoCommand
 ****************************************************************************************/

void CmdNetSignalAdd::redo()
{
    if (!mNetSignal) // only the first time
        mNetSignal = mCircuit.createNetSignal(mNetClass);

    mCircuit.addNetSignal(mNetSignal);
    mIsAdded = true;
}

void CmdNetSignalAdd::undo()
{
    mCircuit.removeNetSignal(mNetSignal);
    mIsAdded = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
