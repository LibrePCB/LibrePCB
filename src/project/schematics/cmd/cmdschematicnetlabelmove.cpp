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
#include "cmdschematicnetlabelmove.h"
#include "../schematicnetlabel.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLabelMove::CmdSchematicNetLabelMove(SchematicNetLabel& netlabel, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Move netlabel"), parent),
    mNetLabel(netlabel), mStartPos(netlabel.getPosition()), mDeltaPos(0, 0),
    mEndPos(netlabel.getPosition()), mStartAngle(netlabel.getAngle()), mDeltaAngle(0),
    mEndAngle(netlabel.getAngle()), mRedoOrUndoCalled(false)
{
}

CmdSchematicNetLabelMove::~CmdSchematicNetLabelMove() noexcept
{
    if (!mRedoOrUndoCalled)
    {
        mNetLabel.setPosition(mStartPos);
        mNetLabel.setAngle(mStartAngle);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdSchematicNetLabelMove::setAbsolutePos(const Point& absPos) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = absPos - mStartPos;
    mNetLabel.setPosition(absPos);
}

void CmdSchematicNetLabelMove::setDeltaToStartPos(const Point& deltaPos) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = deltaPos;
    mNetLabel.setPosition(mStartPos + mDeltaPos);
}

void CmdSchematicNetLabelMove::setAngle(const Angle& angle) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaAngle = angle - mStartAngle;
    mNetLabel.setAngle(angle);
}

void CmdSchematicNetLabelMove::rotate(const Angle& angle, const Point& center) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mDeltaPos = Point(mStartPos + mDeltaPos).rotated(angle, center) - mStartPos;
    mDeltaAngle += angle;
    mNetLabel.setPosition(mStartPos + mDeltaPos);
    mNetLabel.setAngle(mStartAngle + mDeltaAngle);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetLabelMove::redo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::redo(); // throws an exception on error
    mEndPos = mStartPos + mDeltaPos;
    mNetLabel.setPosition(mEndPos);
}

void CmdSchematicNetLabelMove::undo() throw (Exception)
{
    mRedoOrUndoCalled = true;
    UndoCommand::undo();
    mNetLabel.setPosition(mStartPos);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
