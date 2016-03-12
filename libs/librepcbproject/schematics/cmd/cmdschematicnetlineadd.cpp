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
#include "cmdschematicnetlineadd.h"
#include "../schematic.h"
#include "../items/si_netline.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLineAdd::CmdSchematicNetLineAdd(Schematic& schematic, SI_NetPoint& startPoint,
                                               SI_NetPoint& endPoint) noexcept :
    UndoCommand(tr("Add netline")),
    mSchematic(schematic), mStartPoint(startPoint), mEndPoint(endPoint), mNetLine(0)
{
}

CmdSchematicNetLineAdd::~CmdSchematicNetLineAdd() noexcept
{
    if ((mNetLine) && (!isCurrentlyExecuted()))
        delete mNetLine;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdSchematicNetLineAdd::performExecute() throw (Exception)
{
    mNetLine = mSchematic.createNetLine(mStartPoint, mEndPoint, Length(158750)); // can throw

    performRedo(); // can throw

    return true;
}

void CmdSchematicNetLineAdd::performUndo() throw (Exception)
{
    mSchematic.removeNetLine(*mNetLine); // can throw
}

void CmdSchematicNetLineAdd::performRedo() throw (Exception)
{
    mSchematic.addNetLine(*mNetLine); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
