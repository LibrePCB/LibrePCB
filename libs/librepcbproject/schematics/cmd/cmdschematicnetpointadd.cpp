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
#include "cmdschematicnetpointadd.h"
#include "../schematic.h"
#include "../items/si_netpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetPointAdd::CmdSchematicNetPointAdd(Schematic& schematic, NetSignal& netsignal,
                                                 const Point& position, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add netpoint"), parent),
    mSchematic(schematic), mNetSignal(&netsignal), mAttachedToSymbol(false),
    mPosition(position), mSymbolPin(nullptr), mNetPoint(nullptr)
{
}

CmdSchematicNetPointAdd::CmdSchematicNetPointAdd(Schematic& schematic, SI_SymbolPin& pin,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add netpoint"), parent),
    mSchematic(schematic), mNetSignal(nullptr), mAttachedToSymbol(true),
    mPosition(), mSymbolPin(&pin), mNetPoint(nullptr)
{
}

CmdSchematicNetPointAdd::~CmdSchematicNetPointAdd() noexcept
{
    if ((mNetPoint) && (!isExecuted()))
        delete mNetPoint;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetPointAdd::redo() throw (Exception)
{
    if (!mNetPoint) // only the first time
    {
        if (mAttachedToSymbol)
            mNetPoint = mSchematic.createNetPoint(*mSymbolPin); // throws an exception on error
        else
            mNetPoint = mSchematic.createNetPoint(*mNetSignal, mPosition); // throws an exception on error
    }

    mSchematic.addNetPoint(*mNetPoint); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mSchematic.removeNetPoint(*mNetPoint);
        throw;
    }
}

void CmdSchematicNetPointAdd::undo() throw (Exception)
{
    mSchematic.removeNetPoint(*mNetPoint); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mSchematic.addNetPoint(*mNetPoint);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
