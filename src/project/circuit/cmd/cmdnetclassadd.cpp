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
#include "cmdnetclassadd.h"
#include "../netclass.h"
#include "../circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdNetClassAdd::CmdNetClassAdd(Circuit& circuit, const QString& name, QUndoCommand* parent) :
    QUndoCommand(QCoreApplication::translate("CmdNetClassAdd", "Add netclass"), parent),
    mCircuit(circuit), mName(name), mNetClass(0), mIsAdded(false)
{
}

CmdNetClassAdd::~CmdNetClassAdd()
{
    if (!mIsAdded)
        delete mNetClass;
}

/*****************************************************************************************
 *  Inherited from QUndoCommand
 ****************************************************************************************/

void CmdNetClassAdd::redo()
{
    if (!mNetClass) // only the first time
        mNetClass = mCircuit.createNetClass(mName);

    mCircuit.addNetClass(mNetClass);
    mIsAdded = true;
}

void CmdNetClassAdd::undo()
{
    mCircuit.removeNetClass(mNetClass);
    mIsAdded = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
