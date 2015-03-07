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
#include "cmdschematicnetlabeledit.h"
#include "../schematicnetlabel.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLabelEdit::CmdSchematicNetLabelEdit(SchematicNetLabel& netlabel, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit netlabel"), parent),
    mNetLabel(netlabel), mRedoOrUndoCalled(false),
    mNetSignalOld(&netlabel.getNetSignal()), mNetSignalNew(&netlabel.getNetSignal())
{
}

CmdSchematicNetLabelEdit::~CmdSchematicNetLabelEdit() noexcept
{
    if (!mRedoOrUndoCalled)
    {
        mNetLabel.setNetSignal(*mNetSignalOld);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSchematicNetLabelEdit::setNetSignal(NetSignal& netsignal) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mNetSignalNew = &netsignal;
    mNetLabel.setNetSignal(*mNetSignalNew);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetLabelEdit::redo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::redo(); // throws an exception on error
    mNetLabel.setNetSignal(*mNetSignalNew);
}

void CmdSchematicNetLabelEdit::undo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::undo();
    mNetLabel.setNetSignal(*mNetSignalOld);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
