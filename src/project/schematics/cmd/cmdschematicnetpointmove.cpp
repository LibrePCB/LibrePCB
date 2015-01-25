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
#include "cmdschematicnetpointmove.h"
#include "../schematicnetpoint.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetPointMove::CmdSchematicNetPointMove(SchematicNetPoint& point, UndoCommand* parent) throw (Exception) :
    UndoCommand(QCoreApplication::translate("CmdSchematicNetPointMove", "Move netpoint"), parent),
    mNetPoint(point), mStartPos(point.getPosition()), mDeltaPos(0, 0),
    mEndPos(point.getPosition()), mRedoOrUndoCalled(false)
{
}

CmdSchematicNetPointMove::~CmdSchematicNetPointMove() noexcept
{
    if ((!mRedoOrUndoCalled) && (!mDeltaPos.isOrigin()))
        mNetPoint.setPosition(mStartPos);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSchematicNetPointMove::setAbsolutePosTemporary(const Point& absPos) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = absPos - mStartPos;
    mNetPoint.setPosition(absPos);
}

void CmdSchematicNetPointMove::setDeltaToStartPosTemporary(const Point& deltaPos) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = deltaPos;
    mNetPoint.setPosition(mStartPos + mDeltaPos);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetPointMove::redo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::redo(); // throws an exception on error
    mEndPos = mStartPos + mDeltaPos;
    mNetPoint.setPosition(mEndPos);
}

void CmdSchematicNetPointMove::undo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::undo();
    mNetPoint.setPosition(mStartPos);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
