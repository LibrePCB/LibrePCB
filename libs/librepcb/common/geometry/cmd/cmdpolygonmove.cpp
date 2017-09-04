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
#include "cmdpolygonmove.h"
#include "cmdpolygonedit.h"
#include "cmdpolygonsegmentedit.h"
#include "../polygon.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdPolygonMove::CmdPolygonMove(Polygon& polygon) noexcept :
    UndoCommandGroup(tr("Edit polygon"))
{
    mPolygonEditCmd = new CmdPolygonEdit(polygon);
    appendChild(mPolygonEditCmd);

    for (PolygonSegment& segment : polygon.getSegments()) {
        CmdPolygonSegmentEdit* cmd = new CmdPolygonSegmentEdit(segment);
        mSegmentEditCmds.append(cmd);
        appendChild(cmd);
    }
}

CmdPolygonMove::~CmdPolygonMove() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdPolygonMove::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mPolygonEditCmd->setDeltaToStartPos(deltaPos, immediate);
    foreach (CmdPolygonSegmentEdit* cmd, mSegmentEditCmds) { Q_ASSERT(cmd);
        cmd->setDeltaToStartPos(deltaPos, immediate);
    }
}

void CmdPolygonMove::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mPolygonEditCmd->rotate(angle, center, immediate);
    foreach (CmdPolygonSegmentEdit* cmd, mSegmentEditCmds) { Q_ASSERT(cmd);
        cmd->rotate(angle, center, immediate);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
